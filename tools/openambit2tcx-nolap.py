#!/usr/bin/python

""" converts the *.log files produced by openambit in ~/.openambit/ to standard TCX format.
usage: ./openambit2tcx.py inputfile outputFile
required dependecies: python{2,3}-dateutil
"""

#from lxml import etree # does not allow namespace prefixes which are required for gpx extensions; everything else in this script would work otherwise with lxml 
import xml.etree.ElementTree as etree
import math
import sys
import datetime
from dateutil.parser import parse

##############################
## getting input parameters ##
##############################

if len(sys.argv) == 3:
    fileIn=sys.argv[1]
    fileOut=sys.argv[2]
else:
    sys.stderr.write("""\
Convert Openambit *.log files to standard TCX format
usage: {} inputfile outputfile 

Openambit *.log files can normally be found in ~/.openambit/
""".format(sys.argv[0]))
    sys.exit(1)

###########################################
## setting variables up, starting output ##
###########################################

fOut=open(fileOut, 'w')

out='<?xml version="1.0"?>\n\n'
out+='<TrainingCenterDatabase xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns="http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2">\n'
out+='   <Activities>\n'
out+='      <Activity Sport="Other">\n'

rootIn=etree.parse(fileIn)
t_time = rootIn.find("Time")
out+='         <Id>' + t_time.text + 'Z</Id>\n'
out+='         <Lap StartTime="' + t_time.text + 'Z">\n'

header = rootIn.find("Log/Header")
duration = header.findtext("Duration")
out+='            <TotalTimeSeconds>' + str(float(duration) / 1000) + '</TotalTimeSeconds>\n'
distance = header.findtext("Distance")
out+='            <DistanceMeters>' + distance + '</DistanceMeters>\n'
header = rootIn.find("Log/Header/Speed")
speedMax = header.findtext("Max")
out+='            <MaximumSpeed>' + str(float(speedMax) / 3600) + '</MaximumSpeed>\n'
# ?Calories is missing from openambit dump files?
out+='            <Track>\n'

latLast=None
lonLast=None
timeLast=None
altitudeLast=None
hrLast=None
cadenceLast=None
powerLast=None
speedLast=None
tempLast=None
airpressureLast=None
latLatest=None
lonLatest=None
timeGPSLatest=None

lapCount=0
lapArray=[0]
maxLap=0

year=parse(t_time.text, fuzzy=True).year
month=parse(t_time.text, fuzzy=True).month
day=parse(t_time.text, fuzzy=True).day
hour=parse(t_time.text, fuzzy=True).hour
minute=parse(t_time.text, fuzzy=True).minute
second=parse(t_time.text, fuzzy=True).second
#print (str(year) + "\n" + str(month) + "\n" + str(day) + "\n" + str(hour) + "\n" + str(minute) + "\n" + str(second) + "\n")

startTime = datetime.datetime(year, month, day, hour, minute, second)

###########################
## getting activity data ##
###########################

for element in rootIn.iterfind("Log/Samples/Sample"):
    t_trackpoint=etree.Element('Trackpoint')
    t_time=etree.SubElement(t_trackpoint, "Time")
    t_position=etree.SubElement(t_trackpoint, "Position")

    lat=element.findtext("Latitude")
    lon=element.findtext("Longitude")   
    time=float(element.findtext("Time")) / 1000.0
    currentTime=startTime+datetime.timedelta(0, time)
    t_time.text=currentTime.isoformat()+"Z"
    
    altitude=element.findtext("Altitude") if element.findtext("Altitude")!=None else altitudeLast
    hr=element.findtext("HR") if element.findtext("HR")!=None else hrLast
    cadence=element.findtext("Cadence") if element.findtext("Cadence")!=None else cadenceLast
    power=element.findtext("BikePower") if element.findtext("BikePower")!=None else powerLast
    speed=element.findtext("Speed") if element.findtext("Speed")!=None else speedLast
    temp=str(float(element.findtext("Temperature"))/10) if element.findtext("Temperature")!=None else tempLast
    airpressure=element.findtext("SeaLevelPressure") if element.findtext("SeaLevelPressure")!=None else airpressureLast

    sampType=element.findtext("Type")
    if sampType=="lap-info":
        lapType=element.findtext("Lap/Type")
        lapDate=element.findtext("Lap/DateTime")
        lapDuration=element.findtext("Lap/Duration")
        lapDistance=element.findtext("Lap/Distance")
        lapUtc=element.findtext("UTC")
        lapPreviousLat=latLatest
        lapPreviousLon=lonLatest
        lapPreviousTime=timeGPSLatest
        lapCheck=1

        if lapCount==0:
            lapArray[0]=[lapType,lapDate,lapDuration,lapDistance,lapUtc,lapPreviousLat,lapPreviousLon,lapPreviousTime,0,0,0]
        else:
            lapArray.append([lapType,lapDate,lapDuration,lapDistance,lapUtc,lapPreviousLat,lapPreviousLon,lapPreviousTime,0,0,0])
        lapCount+=1

    maxLap=lapCount-1

    if lat!=None and lon!=None:
        t_time
        
        lat=float(lat)/10000000
        lon=float(lon)/10000000

        t_latitudeDegrees=etree.SubElement(t_position, "LatitudeDegrees")
        t_latitudeDegrees.text=str(lat)
        t_longitudeDegrees=etree.SubElement(t_position, "LongitudeDegrees")
        t_longitudeDegrees.text=str(lon)

        latLatest=str(lat)
        lonLatest=str(lon)
        
        if lapCheck==1:
            lapArray[lapCount-1][8]=latLatest
            lapArray[lapCount-1][9]=lonLatest
            lapArray[lapCount-1][10]=timeGPSLatest
            lapCheck=0
        
        if altitude!=None:
            etree.SubElement(t_trackpoint,"AltitudeMeters").text=altitude 
        elif altitudeLast!=None:
            etree.SubElement(t_trackpoint,"AltitudeMeters").text=altitudeLast
            
        if hr!=None or cadence!=None or power!=None or speed!=None or temp!=None or airpressure!=None:
            t_extGpx=etree.SubElement(t_trackpoint,"Extensions")
            t_tpx=etree.SubElement(t_extGpx,"TPX")
            t_tpx.set("xmlns","http://www.garmin.com/xmlschemas/ActivityExtension/v2")
            if hr!=None: etree.SubElement(t_tpx,"HR").text=hr
            if cadence!=None: etree.SubElement(t_tpx,"Cadence").text=cadence
            if power!=None: etree.SubElement(t_tpx,"Power").text=power
            if temp!=None: etree.SubElement(t_tpx,"Temp").text=temp
            if speed!=None: etree.SubElement(t_tpx,"Speed").text=str(float(speed)/100.0)
            if airpressure!=None: etree.SubElement(t_tpx,"SeaLevelPressure").text=airpressure
        
        if altitude!=None:
            out+="               "+etree.tostring(t_trackpoint)+"\n"
        
    
    latLast=lat
    lonLast=lon
    timeLast=time
    altitudeLast=altitude
    hrLast=hr
    cadenceLast=cadence
    powerLast=power
    speedLast=speed
    tempLast=temp
    airpressureLast=airpressure

    lat=None
    lon=None
    time=None
    altitude=None
    hr=None
    cadence=None
    power=None
    speed=None
    temp=None
    airpressure=None    
        
        
out+='            </Track>\n'
out+='         </Lap>\n'
out+='      </Activity>\n'
out+='   </Activities>\n'
out+='</TrainingCenterDatabase>\n'

fOut.write(out)

fOut.close()
