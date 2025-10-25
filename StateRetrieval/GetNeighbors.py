import asyncio
import time
import httpx
import time
import numpy
import signal
from datetime import datetime
import pandas as pd
import json


connected = []

def handler(signum, frame):
    res = input("Ctrl-c was pressed. Do you really want to exit? y/n ")
    if res == 'y':
        exit(1)

async def call_url_post(session, ipN, uri,json_body):
    global connected
    url = "http://192.168."+ipN+"/"+uri
    #print("Connecting to "+ ipN)
    response = None

    try:       
        response = await session.request(method='POST', url=url,data=json_body, timeout=None)
        #response.raise_for_status()
        print("Response from "+ipN,response.status_code)
        if response.status_code == httpx.codes.OK:
            #print(response.text)
            if ipN not in connected:
                connected.append(ipN)
            #print(response.json()) 
            return response.json()
        else:
            print("Not correct response from " + ipN)
    except(httpx.ConnectError, httpx.ConnectTimeout) as e:
        print(f"ConnectError {url}")

async def call_url_get(session, ipN, uri):
    global connected
    url = "http://192.168."+ipN+"/"+uri
    #print("Connecting to "+ ipN)
    response = None

    try:       
        response = await session.request(method='GET', url=url, timeout = None)
        #response.raise_for_status()
        print("Response from "+ipN,response.status_code)
        if response.status_code == httpx.codes.OK:
            #print(response.text)
            if ipN not in connected:
                connected.append(ipN)
            #print(response.json()) 
            return response.json()
        else:
            print("Not correct response from " + ipN)
    except Exception as e:
        print(f"ConnectError {url}")


async def concurrent_post(uri:str,json_body):
    global connected
    limits = httpx.Limits(max_keepalive_connections=None, max_connections=None)
    tasks = []
    async with httpx.AsyncClient(limits = limits) as session:  #use httpx
        startTime = time.time()
        session.headers.update({'Content-Type': 'application/json'})
        for x in connected:
            tasks.append(asyncio.create_task(call_url_post(session,x,uri,json_body)))       
        responses = await asyncio.gather(*tasks)
        print("Elapsed time for all responses: " + str(time.time()-startTime))
        return responses
        
async def concurrent_get(uri:str):
    global connected
    limits = httpx.Limits(max_keepalive_connections=None, max_connections=None)
    tasks = []
    async with httpx.AsyncClient(limits = limits) as session:  #use httpx
        startTime = time.time()
        if len(connected) > 0:
            for x in connected:
                tasks.append(asyncio.create_task(call_url_get(session,x,uri)))
        else:
            for i in range(72,74):
                for j in range(0,256):
                    if (i == 72 and j >=2) or i == 73:
                        tasks.append(asyncio.create_task(call_url_get(session,str(i)+"."+str(j),uri)))          
        responses = await asyncio.gather(*tasks)
        print(connected)
        print("Number of connected cubes: "+str(len(connected)))
        print("Elapsed time for all responses: " + str(time.time()-startTime))
        return responses
    
signal.signal(signal.SIGINT, handler)

def attempt_to_connect(attempts,expected,start_time,file_name):
    global connected
    for i in range(attempts):
        requestTime = datetime.now()
        data = (asyncio.run(concurrent_get(uri="state_guess")))
        if len(connected)>0:
            responseTime = datetime.now()
            experimentTime = time.time()-start_time
            dataTable = []
            for response in data:
                if response:
                    row = [experimentTime,requestTime,responseTime,response["fail_state"],response["macId"],response["mac0"],response["mac1"],response["mac2"],response["mac3"],response["mac4"],response["mac5"],response["mac6"],response["mac7"],response["update_num"],response["state_guess"],response["firmware"]]
                    dataTable.append(row)

        #print(dataTable)

        #with open(file_name, "ab") as f:
            #numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')
        
        if len(connected)>=expected:
            break
        else:
            print("Not all expected cubes connected yet")
            #time.sleep() #There is enough timeout already with the connection timeout

    if len(connected) < expected:
        print("All "+ str(expected) +" cubes did not connect after "+str(attempts)+" attempts")
        exit()

def reset_buffers(start_time):
    global connected
    requestTime = datetime.now()
    data = (asyncio.run(concurrent_get(uri="reset-buffers")))
    if len(connected)>0:
        responseTime = datetime.now()
        experimentTime = time.time()-start_time
        dataTable = []
        for response in data:
            if response:
                row = [experimentTime,requestTime,responseTime,response["resetBuffers"]]
                dataTable.append(row)

        #print(dataTable)

def getNeighborInfo(repetitions,file_name,expected,color):
    global connected
    retry = 0

    for i in range(repetitions):
        requestTime = datetime.now()
        if not color:
            data = (asyncio.run(concurrent_get(uri="morphology")))
        else:
            data = (asyncio.run(concurrent_get(uri="morphology?color=1")))
        if len(connected)>0:
            responseTime = datetime.now()
            dataTable = []
            for response in data:
                if response:
                    row = [requestTime,responseTime,
                        response["MyID"],
                        hex(int(response["MyID0"])),
                        hex(int(response["MyID1"])),
                        hex(int(response["MyID2"])),
                        hex(int(response["MyID3"])),
                        hex(int(response["MyID4"])),
                        hex(int(response["MyID5"])),
                        hex(int(response["MyID6"])),
                        hex(int(response["MyID7"])),
                        hex(int(response["WEST0"])),
                        hex(int(response["WEST1"])),
                        hex(int(response["WEST2"])),
                        hex(int(response["WEST3"])),
                        hex(int(response["WEST4"])),
                        hex(int(response["WEST5"])),
                        hex(int(response["WEST6"])),
                        hex(int(response["WEST7"])),
                        hex(int(response["EAST0"])),
                        hex(int(response["EAST1"])),
                        hex(int(response["EAST2"])),
                        hex(int(response["EAST3"])),
                        hex(int(response["EAST4"])),
                        hex(int(response["EAST5"])),
                        hex(int(response["EAST6"])),
                        hex(int(response["EAST7"])),
                        hex(int(response["NORTH0"])),
                        hex(int(response["NORTH1"])),
                        hex(int(response["NORTH2"])),
                        hex(int(response["NORTH3"])),
                        hex(int(response["NORTH4"])),
                        hex(int(response["NORTH5"])),
                        hex(int(response["NORTH6"])),
                        hex(int(response["NORTH7"])),
                        hex(int(response["SOUTH0"])),
                        hex(int(response["SOUTH1"])),
                        hex(int(response["SOUTH2"])),
                        hex(int(response["SOUTH3"])),
                        hex(int(response["SOUTH4"])),
                        hex(int(response["SOUTH5"])),
                        hex(int(response["SOUTH6"])),
                        hex(int(response["SOUTH7"])),
                        hex(int(response["FRONT0"])),
                        hex(int(response["FRONT1"])),
                        hex(int(response["FRONT2"])),
                        hex(int(response["FRONT3"])),
                        hex(int(response["FRONT4"])),
                        hex(int(response["FRONT5"])),
                        hex(int(response["FRONT6"])),
                        hex(int(response["FRONT7"])),
                        hex(int(response["BACK0"])),
                        hex(int(response["BACK1"])),
                        hex(int(response["BACK2"])),
                        hex(int(response["BACK3"])),
                        hex(int(response["BACK4"])),
                        hex(int(response["BACK5"])),
                        hex(int(response["BACK6"])),
                        hex(int(response["BACK7"])),
                        retry,
                        expected,
                        len(connected)]
                    dataTable.append(row)
                    

            #print(dataTable)
            retry += 1
            

            with open(file_name, "ab") as f:
                numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')
            
            time.sleep(10)
        
            
        else:
            print("Cubes not connected")
            #time.sleep() #There is enough timeout already with the connection timeout




def getNeighbors(initial_connection_attempts=10 ,repetitions = 10,file_name="testNeighbors.csv", expected = 26):

    start_time = time.time()

    attempt_to_connect(initial_connection_attempts,expected,start_time=start_time,file_name=file_name)

    #print("Resetting buffers")
    #reset_buffers(start_time)
    #time.sleep(3)

    print("Getting neighbor info")
    getNeighborInfo(repetitions,file_name,expected,color=False)

    print("Finished getting neighbor information")

def main():
    getNeighbors(expected=33,initial_connection_attempts=10,repetitions=10,file_name="testNeighbors_Boat_144_"+str(time.time())+".csv")

if __name__ == "__main__":
    main()