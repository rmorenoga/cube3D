import random
import uploadAsyncElegantOTA as asyncOTA
import espota as basicOTA
import subprocess
import os
import httpx
import time
import asyncio
from datetime import datetime
from joblib import Parallel, delayed

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

async def concurrent_get(firstTime:bool,firmware_version, server_IP = 21):
    global connected
    global old_version_cubes
    limits = httpx.Limits(max_keepalive_connections=None, max_connections=None)
    tasks = []
    async with httpx.AsyncClient(limits = limits) as session:  #use httpx
        if firstTime:
            startTime = time.time()
            for i in range(72,74): #Change according to subnet
                for j in range(0,256): #Change according to subnet
                    if (i == 72 and (j >=2 and j!= server_IP)) or (i == 73 and j < 255):
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
        

def uploadFromSlicedList(cubes_to_program):
    firmwarePath="../3DCubePlatformIO/.pio/build/adafruit_metro_esp32s2/firmware.bin"
    otaType="asyncOTA"
    timeout=10
    for i in cubes_to_program:
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

def chunks(lst, n):
  result = []
  for i in range(0, len(lst), n):
      result.append(lst[i:i + n])
  return result


def uploadFromList(network, firmwarePath, otaType, timeout = 10,version_filter = True,firmware_version="2.0",sliceSize=1, server_IP = 21):
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

   
    # if os.name == 'nt':
    #     wifi = subprocess.check_output(['netsh', 'WLAN', 'show', 'interfaces'])
    #     data = wifi.decode('utf-8')
    # else:
    #     wifi = subprocess.check_output(['sudo', 'iwgetid'])
    #     data = wifi.decode('utf-8')

    # if network in data:
    #     print("Connected to network: " + network)
    num_programmed = 0
    


    attempts = 2
    for i in range(attempts):
        requestTime = datetime.now()
        data = (asyncio.run(concurrent_get(firstTime=True,firmware_version=firmware_version,server_IP=server_IP)))
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

    slicedList = chunks(cubes_to_program,sliceSize)

    Parallel(n_jobs=-1)(delayed(uploadFromSlicedList)(sliced) for sliced in slicedList)
       

        #uploadFromList(cubes_to_program,firmwarePath,otaType,timeout)
        
    # else:
    #     print("Not connected to network: " + network)



uploadFromList("sensors","../3DCubePlatformIO/.pio/build/adafruit_metro_esp32s2/firmware.bin","asyncOTA",version_filter=True,firmware_version="4.98",sliceSize=1, server_IP = 22)
#help(uploadFromList)