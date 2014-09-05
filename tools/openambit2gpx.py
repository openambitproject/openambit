#!/usr/bin/python

""" concerts the *.log files produced by openambit in ~/.openambit/ to standard gpx format.
usage: ./openambit2gpx.py inputfile outputFile
"""

#from lxml import etree # does not allow namespace prefixes which are required for gpx extensions; everything else in this script would work otherwise with lxml 
import xml.etree.ElementTree as etree
import math
import sys

##############################
## getting input parameters ##
##############################

fileIn=sys.argv[1]
fileOut=sys.argv[2]


###########################################
## setting variables up, starting output ##
###########################################

fOut=open(fileOut, 'w')

fOut.write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n\n")
fOut.write('<gpx xmlns="http://www.topografix.com/GPX/1/1" version="1.1" creator="Ascent 1.11.9" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:gpxdata="http://www.cluetrust.com/XML/GPXDATA/1/0" xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd http://www.cluetrust.com/XML/GPXDATA/1/0 http://www.cluetrust.com/Schemas/gpxdata10.xsd">')
fOut.write(" <trk>\n")
fOut.write("  <trkseg>\n")

rootIn=etree.parse(fileIn)

latLast=None
lonLast=None
timeLast=None
altitudeLast=None
hrLast=None
cadenceLast=None
speedLast=None
tempLast=None
airpressureLast=None
latLatest=None
lonLatest=None
timeGPSLatest=None

lapCount=0
lapArray=[0]
maxLap=0

def utcSplitConvSeconds(utcTime):
    """ Splits the UTC time code YYYY-MM-DDTHH:MM:SS.SSSZ, keeps only the time part and converts it into seconds.
    """

    import math
    tmpTime=utcTime.split("T")[1].split("Z")[0].split(":")
    tmpDay=int(utcTime.split("T")[0].split("-")[2])
    secs=float(tmpDay)*24*3600 + float(tmpTime[0])*3600 + float(tmpTime[1])*60 + float(tmpTime[2])

    return secs

def timeDiff(utcTime1,utcTime2):
    """ Computes the difference, in seconds, between an earlier (utcTime1) and a later date (utcTime2). Only safe for dates within the same month or less than 2 days apart if on the boundary of a month.
    """

    secs1=utcSplitConvSeconds(utcTime1)
    secs2=utcSplitConvSeconds(utcTime2)

    if int(utcTime2.split("T")[0].split("-")[2])==1:
        secs1-=float(utcTime2.split("T")[0].split("-")[2])*24*3600 # if second date is on a first of a month, then the previous day gets reset to day 0 of the same month

    return secs2-secs1


###########################
## getting activity data ##
###########################

for element in rootIn.iterfind("Log/Samples/Sample"):
    trk=etree.Element("trkpt")

    lat=element.findtext("Latitude")
    lon=element.findtext("Longitude")
    time=element.findtext("UTC")

    altitude=element.findtext("Altitude") if element.findtext("Altitude")!=None else altitudeLast
    hr=element.findtext("HR") if element.findtext("HR")!=None else hrLast
    cadence=element.findtext("Cadence") if element.findtext("cadence")!=None else cadenceLast
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
        lat=float(lat)/10000000
        lon=float(lon)/10000000

        trk.set("lat",str(lat))
        trk.set("lon",str(lon))

        latLatest=str(lat)
        lonLatest=str(lon)
        timeGPSLatest=time

        if lapCheck==1:
            lapArray[lapCount-1][8]=latLatest
            lapArray[lapCount-1][9]=lonLatest
            lapArray[lapCount-1][10]=timeGPSLatest
            lapCheck=0
            
        if altitude!=None:
            etree.SubElement(trk,"ele").text=altitude 
        elif altitudeLast!=None:
            etree.SubElement(trk,"ele").text=altitudeLast
 
        if time!=None:
            etree.SubElement(trk,"time").text=time 
        elif timeLast!=None:
            etree.SubElement(trk,"time").text=timeLast 

        if hr!=None or cadence!=None or speed!=None or temp!=None or airpressure!=None:
            extGpx=etree.SubElement(trk,"extensions")
            if hr!=None: etree.SubElement(extGpx,"gpxdata:hr").text=hr
            if cadence!=None: etree.SubElement(extGpx,"gpxdata:cadence").text=cadence
            if temp!=None: etree.SubElement(extGpx,"gpxdata:atemp").text=temp
            if speed!=None: etree.SubElement(extGpx,"gpxdata:speed").text=speed
            if airpressure!=None: etree.SubElement(extGpx,"gpxdata:SeaLevelPressure").text=airpressure

        fOut.write("   "+etree.tostring(trk)+"\n")

    latLast=lat
    lonLast=lon
    timeLast=time
    altitudeLast=altitude
    hrLast=hr
    cadenceLast=cadence
    speedLast=speed
    tempLast=temp
    airpressureLast=airpressure

    lat=None
    lon=None
    time=None
    altitude=None
    hr=None
    cadence=None
    speed=None
    temp=None
    airpressure=None

fOut.write("  </trkseg>\n")
fOut.write(" </trk>\n")


#############################
## getting lap information ##
#############################

lapCount=0
previousEndTime=0

fOut.write(" <extensions>\n")

for i in range(0,len(lapArray)):
    if lapArray[i][0]=='Manual':
        lap=etree.Element("gpxdata:lap")
        lap.set("xmlns","http://www.cluetrust.com/XML/GPXDATA/1/0")

        startTime=lapArray[0][4] if lapCount==0 else previousEndTime
        previousEndTime=lapArray[i][4]

        etree.SubElement(lap,'index').text=str(lapCount)
        etree.SubElement(lap,'startTime').text=startTime
        etree.SubElement(lap,'elapsedTime').text=str(float(lapArray[i][2])/1000)
        etree.SubElement(lap,'distance').text=lapArray[i][3]

        latInterPolSP=lapArray[0][8] if lapCount==0 else previousLatEP
        lonInterPolSP=lapArray[0][9] if lapCount==0 else previousLonEP

        if i==maxLap:
            latInterPolEP=lapArray[i][5]
        else:
            t=lapArray[i][4]
            t1=lapArray[i][7]
            t2=lapArray[i][10]
            lat1=float(lapArray[i][5])
            lat2=float(lapArray[i][8])
            latInterPolEP=str( ((lat2-lat1)/timeDiff(t1,t2))*timeDiff(t1,t) + lat1 )
        if i==maxLap:
            lonInterPolEP=lapArray[i][6]
        else:
            t=lapArray[i][4]
            t1=lapArray[i][7]
            t2=lapArray[i][10]
            lon1=float(lapArray[i][6])
            lon2=float(lapArray[i][9])
            lonInterPolEP=str( ((lon2-lon1)/timeDiff(t1,t2))*timeDiff(t1,t) + lon1 )
        previousLatEP=latInterPolEP
        previousLonEP=lonInterPolEP
        SP=etree.SubElement(lap,'startPoint')
        SP.text=' '
        SP.set('lat',str(latInterPolSP))
        SP.set('lon',str(lonInterPolSP))
        EP=etree.SubElement(lap,'endPoint')
        EP.text=' '
        EP.set('lat',str(latInterPolEP))
        EP.set('lon',str(lonInterPolEP))

        etree.SubElement(lap,'intensity').text='active'
        trigger=etree.SubElement(lap,'trigger')
        trigger.text=' '
        trigger.set('kind','manual')

        #the elements below need to be added for the gpx file to be understood; data all set to 0
        etree.SubElement(lap,'calories').text='0'
        sumHrAvg=etree.SubElement(lap,'summary')
        sumHrAvg.text='0'
        sumHrAvg.set('kind','avg')
        sumHrAvg.set('name','hr')
        sumHrMax=etree.SubElement(lap,'summary')
        sumHrMax.text='0'
        sumHrMax.set('kind','max')
        sumHrMax.set('name','hr')
        sumCadAvg=etree.SubElement(lap,'summary')
        sumCadAvg.text='0'
        sumCadAvg.set('kind','avg')
        sumCadAvg.set('name','cadence')
        sumSpeedMax=etree.SubElement(lap,'summary')
        sumSpeedMax.text='0'
        sumSpeedMax.set('kind','max')
        sumSpeedMax.set('name','speed')
        
        lapCount+=1

        fOut.write("  "+etree.tostring(lap)+"\n")

fOut.write(" </extensions>\n")


#########################
## closing output file ##
#########################

fOut.write("</gpx>\n")
fOut.close()
