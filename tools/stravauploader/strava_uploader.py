#!/usr/bin/env python3

import requests
import argparse
import time
import sys
from datetime import datetime
from http.server import *
from threading import Thread

try:
    import config
    if config.CLIENT_ID == 00000 or\
       config.CLIENT_SECRET == '0000000000000000000000000000000000000000' or\
       config.ACCESS_TOKEN == '0000000000000000000000000000000000000000':
        raise Exception('Invalid value')
except:
    print('Invalid value in config.py')
    sys.exit(1)

HTTP_PORT=8982

class StopServer(Thread):

    def __init__(self, server):
        self.server = server
        super().__init__()
        
    def run(self):
        self.server.shutdown()

class RequestHandler(SimpleHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()

        # print(self.path)
        args=self.path.split('?')
        if len(args) != 2:
            self.wfile.write('No args specified\n'.encode('utf-8'))
            self.server.shutdown()
        args = dict(x.split("=") for x in args[1].split("&"))
        if not 'activity:write' in args['scope']:
            self.wfile.write('You must accept activity write\n'.encode('utf-8'))
            return
        
        headers={}
        params = {'client_id': config.CLIENT_ID,
                  'client_secret': config.CLIENT_SECRET,
                  'code':args['code'],
                  'grant_type':'authorization_code'}
        url='https://www.strava.com/oauth/token'
        r = requests.post(url, headers=headers, params=params)
        json_vals = r.json()
        # print(json_vals)

        message = 'Your new access token is {}, please update config.py'.format(json_vals['access_token'])
        print(message)
        self.wfile.write(message.encode('utf-8'))
        config.ACCESS_TOKEN = json_vals['access_token']
        self.send_response(200)
        stop = StopServer(self.server)
        stop.start()
        
def uploadMove(activity):
    url = 'https://www.strava.com/api/v3/uploads'
    ext = activity[-3:]
    if not ext in ['fit', 'tcx', 'gpx']:
        ext = activity[-6:]
        if not ext in ['fit.gz', 'tcx.gz', 'gpx.gz']:
            return (-1, 'Invalid file {}, must be one with extension fit[.gz], tcx[.gz] or gpx[.gz]'.format(activity))
        
    payload = {'data_type': ext}

    print('Process {}'.format(activity))

    files = {'file': open(activity, 'rb')}
    headers = {'Authorization': 'Bearer '+config.ACCESS_TOKEN}
    r = requests.post(url, headers=headers, params=payload, files=files)
    # print(r.json())
    # print(r.status_code)

    resp = r.json()

    if (r.status_code == 401):
        return (r.status_code, resp['message'])
    if (r.status_code != 201 and r.status_code != 200):
        print(r.json())
        print(r.status_code)
        return (r.status_code, resp['status'])

    while resp['activity_id'] == None:
        time.sleep(1)
        activty_url = url + '/' + str(resp['id'])
        r = requests.get(activty_url, headers=headers)
        resp = r.json()

    if r.status_code == 200:
        print('New activity at https://www.strava.com/activities/{}'.format(resp['activity_id']))
        return (r.status_code, 'OK')
    else:
        return (r.status_code, resp['message'])

def uploadMoves(activities):
    i = 0
    for activity in activities:
        (status_code, status) = uploadMove(activity)
        if status_code != 201:
            now = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
            if status_code == 200:
                print('Strava code {}, status {}'.format(status_code, status))
            elif status_code == 401:
                print('Go to https://www.strava.com/oauth/authorize?client_id={}&redirect_uri=http://localhost:{}&response_type=code&approval_prompt=auto&scope=activity:write'.format(config.CLIENT_ID, HTTP_PORT))
                httpd = HTTPServer(('127.0.0.1', HTTP_PORT), RequestHandler)
                httpd.serve_forever()
                # Try again
                uploadMove(activity)
            else:
                print('error at {}, code {}, status {}'.format(now, status_code, status))

        i += 1
        if i == 100:  # 100 files per 15 mins rate limit
            print('Limit rate raised, exit')
            break

parser = argparse.ArgumentParser()
parser.add_argument("-l", "--logs", nargs="+", help="Logs to upload",
                    dest="logs", required=True)
args = parser.parse_args()

uploadMoves(args.logs)
