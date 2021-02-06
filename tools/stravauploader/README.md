This script upload your logs to Strava website using Strava API v3.

Installation
============

No special dependancy is needed except Python3 interpreter.

First, you have to fill _config.py_ with the right values :

  * CLIENT_ID
  * CLIENT_SECRET
  * ACCESS_TOKEN

To get them, go to https://www.strava.com/settings/api

Create an application, then copy your values


Usage
=====

    strava_uploader.py -l LOGS [LOGS ...]

Valid logs are *.fit, *.tcx and *.gpx


Git repository
==============

https://github.com/soutade/StravaUploader


Licence
=======

GNU GPL v3