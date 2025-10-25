import random
import uploadAsyncElegantOTA as asyncOTA
import espota as basicOTA
import subprocess
import os
import httpx
import time
import asyncio
from datetime import datetime

connected = []
old_version_cubes = []

async def call_url(session, ipN,firmware_version):
    global connected
    global old_version_cubes
    url = "http://192.168."+ipN+"/state_guess"
    print("Connecting to "+ ipN)
    response = None

    try:       
        response = await session.request(method='GET', url=url,timeout=20)
        #response.raise_for_status()
        print("Response from "+ipN,response.status_code)
        if response.status_code == httpx.codes.OK:
            #print(response.text)
            if ipN not in connected:
                connected.append(ipN)

            if response.json()["firmware"] != firmware_version:
                if ipN not in old_version_cubes:
                    old_version_cubes.append(ipN)

            #print(response.json()) 
            return response.json()
        else:
            print("Not correct response from " + ipN)
    except(httpx.ConnectError, httpx.ConnectTimeout) as e:
        print(f"ConnectError {url}")
    except httpx.HTTPError as e:
        print(f"ConnectError {url}")

async def concurrent_get(firstTime:bool,firmware_version):
    global connected
    global old_version_cubes
    limits = httpx.Limits(max_keepalive_connections=None, max_connections=None)
    tasks = []
    async with httpx.AsyncClient(limits = limits) as session:  #use httpx
        if firstTime:
            startTime = time.time()
            for i in range(72,74):
                for j in range(0,256):
                    if (i == 72 and j >=2) or i == 73:
                        tasks.append(asyncio.create_task(call_url(session,str(i)+"."+str(j),firmware_version)))          
            responses = await asyncio.gather(*tasks)
            print(connected)
            print(old_version_cubes)
            print(time.time()-startTime)
            return responses
        else:
            startTime = time.time()
            for x in connected:
                tasks.append(asyncio.create_task(call_url(session,x,firmware_version)))       
            responses = await asyncio.gather(*tasks)
            print(connected)
            print(old_version_cubes)
            print(time.time()-startTime)
            return responses

def uploadFromList(network, firmwarePath, otaType, timeout = 10,skipList = [],version_filter = True,firmware_version="2.0"):
    global connected
    global old_version_cubes
    """A function that uploads firmware to multiple ESP32s OTA
       
    Parameters
    ----------
    iPRangeStart : int
        The start of the ip range in which the ESPs are connected
    iPRangeEnd : int
        The end of the ip range in which the ESPs are connected
    skipList : int[]
        An array of ints with the ip terminators to skip while uploading
    firmwarePath : str
        The path of the firmware file to upload
    otaType : str
        The type of OTA script to use:
            basicOTA: OTA script used for the BasicOTA arduino example (extracted from the arduino IDE)
            asyncOTA: OTA script used for the AsyncElegantOTA server (extracted from platformIO) 
    timeout : int, optional
        The time out time in seconds for connecting to the ESPs and reading a response (default is 10)
    """

   
    if os.name == 'nt':
        wifi = subprocess.check_output(['netsh', 'WLAN', 'show', 'interfaces'])
        data = wifi.decode('utf-8')
    else:
        wifi = subprocess.check_output(['sudo', 'iwgetid'])
        data = wifi.decode('utf-8')

    if network in data:
        print("Connected to network: " + network)
        num_programmed = 0
        


        attempts = 2
        for i in range(attempts):
            requestTime = datetime.now()
            data = (asyncio.run(concurrent_get(firstTime=True,firmware_version=firmware_version)))
            if len(connected)>0:
                responseTime = datetime.now()
                dataTable = []
                for response in data:
                    if response:
                        row = [requestTime,responseTime,response["macId"],response["mac0"],response["mac1"],response["mac2"],response["mac3"],response["mac4"],response["mac5"],response["mac6"],response["mac7"],response["update_num"],response["state_guess"]]
                        dataTable.append(row)


                print("Connected cubes ",len(connected), connected)
                print("Cubes with old version firmware ", len(old_version_cubes), old_version_cubes)
            else:
                print("Cubes not connected")
                #time.sleep() #There is enough timeout already with the connection timeout

        if len(connected) <=0:
            print("Cubes didnt connect after 10 attempts")
            exit()

        
        
        if (version_filter):
            cubes_to_program = old_version_cubes
        else:
            cubes_to_program = connected

        #cubes_to_program =['72.22', '72.28', '72.37', '72.23', '72.35', '72.42', '72.25', '72.26', '72.66', '72.45', '72.27', '72.44', '72.76', '72.30', '72.80', '72.65', '72.73', '72.84', '72.54', '72.78']
        #cubes_to_program =['72.56', '72.92', '72.88', '72.97', '72.111', '72.60', '72.109', '72.59', '72.112', '72.130', '72.24', '72.141', '72.38', '72.134', '72.142', '72.108', '72.140', '72.155', '72.159', '72.174']
        #cubes_to_program =['72.68', '72.168', '72.33', '72.31', '72.40', '72.205', '72.29', '72.212', '72.128', '72.71', '72.189', '72.147', '72.85', '72.110', '72.99', '72.148', '72.46', '72.21', '72.34', '72.36']
        #cubes_to_program =['72.32', '72.52', '72.39', '72.41', '72.47', '72.43', '72.48', '72.49', '72.50', '72.51', '72.150', '72.156', '72.160', '72.171', '72.182', '72.61', '72.185', '72.191', '72.53', '72.194']
        #cubes_to_program =['72.200', '72.202', '72.217', '72.57', '72.136', '72.55', '72.58', '72.63', '72.62', '72.72', '72.64', '72.74', '72.77', '72.69', '72.149', '72.67', '72.75', '72.79', '72.95', '72.83']
        #cubes_to_program =['72.82', '72.81', '72.86', '72.91', '72.89', '72.87', '72.93', '72.98', '72.94', '72.100', '72.101', '72.103', '72.104', '72.105', '72.107', '72.116', '72.115', '72.106', '72.120', '72.113']
        #cubes_to_program =['72.123', '72.117', '72.102', '72.124', '72.126', '72.131', '72.125', '72.133', '72.135', '72.132', '72.139', '72.137', '72.129', '72.143', '72.138', '72.146', '72.145', '72.144', '72.152', '72.154']
        #cubes_to_program =['72.153', '72.118', '72.157', '72.161', '72.119', '72.162', '72.165', '72.170', '72.114', '72.122', '72.176', '72.127', '72.173', '72.169', '72.177', '72.167', '72.180', '72.207', '72.183', '72.163']
        #cubes_to_program =['72.209', '72.158', '72.195', '72.215', '72.193', '72.216', '72.203', '72.214', '72.190', '72.213', '72.187', '72.210', '72.211', '72.164', '72.199', '72.186', '72.201', '72.188', '72.206', '72.204']
        #cubes_to_program =['72.151', '72.192', '72.178', '72.166', '72.197', '72.181', '72.208', '72.172', '72.175', '72.90', '72.70', '72.184', '72.96', '72.198', '72.121', '72.179', '72.196']
        #cubes_to_program =['72.179']

        

        for i in cubes_to_program:
            if i not in skipList:
                print("192.168."+i)
                try:
                    if(otaType=='asyncOTA'):
                        asyncOTA.upload(firmwarePath,"http://192.168."+i+"/update",timeout)
                        num_programmed = num_programmed + 1
                        print(num_programmed)
                       
                    if(otaType=='basicOTA'):
                        basicOTA.TIMEOUT = timeout
                        basicOTA.serve("192.168."+i, "0.0.0.0", 3232, random.randint(10000,60000), "", firmwarePath)
                except:
                    print("Exception caught")
        

    else:
        print("Not connected to network: " + network)



uploadFromList("sensors","../3DCubePlatformIO/.pio/build/adafruit_metro_esp32s2/firmware.bin","asyncOTA",version_filter=True,firmware_version="2.55")
#help(uploadFromList)
