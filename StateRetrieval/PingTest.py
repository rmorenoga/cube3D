import asyncio
import time
import httpx
import time
import numpy
from datetime import datetime
import signal
import requests
import asyncio
import aioping

connected = []
rNUmber = 0

def handler(signum, frame):
    res = input("Ctrl-c was pressed. Do you really want to exit? y/n ")
    if res == 'y':
        exit(1)

async def call_url(session, ipN, uri):
    global connected
    global rNUmber
    url = "http://192.168."+ipN+"/"+uri
    #print("Connecting to "+ ipN)
    response = None
    try:       
        response = await session.request(method='GET', url=url,timeout=2)
        #response.raise_for_status()
        print("Response from "+ipN,str(response.status_code) + " " +str(rNUmber))
        if response.status_code == httpx.codes.OK:
            rNUmber += 1
            #print(response.text)
            if ipN not in connected:
                connected.append(ipN)
            #print(response.json()) 
            if response.headers.get('content-type') =='text/plain':
                return response.text
            else:
                return response.json()
        else:
            print("Not correct response from " + ipN)
            return str(ipN)
    except httpx.TimeoutException as e:
        print(f"Timeout Error {url}")
        return str(ipN)
    except httpx.ProtocolError as e:
        print(f"Protocol Error {url}")
        return str(ipN)
    except httpx.NetworkError as e:
        print(f"Network Error {url}")
        return str(ipN)

def call_url_nc(ipN, uri):
    global connected
    url = "http://192.168."+ipN+"/"+uri
    #print("Connecting to "+ ipN)
    response = None
    try:
        response = requests.get(url=url,timeout=1)       
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
        print(f"Exception at {url}")

async def concurrent_get(firstTime:bool,uri:str,cube_list=None):
    global connected
    global rNUmber
    limits = httpx.Limits(max_keepalive_connections=None, max_connections=None)
    tasks = []
    rNUmber=0
    async with httpx.AsyncClient(limits = limits) as session:  #use httpx
        if cube_list is not None:
            startTime = time.time()
            print("Contacting only :"+str(cube_list))
            for x in cube_list:
                tasks.append(asyncio.create_task(call_url(session,x,uri)))       
            responses = await asyncio.gather(*tasks)
            print("Elapsed time for all responses: " + str(time.time()-startTime))
            return responses
        elif firstTime:
            startTime = time.time()
            for i in range(72,74):
                for j in range(0,256):
                    if (i == 72 and j >=2) or i == 73:
                        tasks.append(asyncio.create_task(call_url(session,str(i)+"."+str(j),uri)))          
            responses = await asyncio.gather(*tasks)
            print(connected)
            print("Number of connected cubes: "+str(len(connected)))
            print("Elapsed time for all responses: " + str(time.time()-startTime))
            return responses
        else:
            startTime = time.time()
            for x in connected:
                tasks.append(asyncio.create_task(call_url(session,x,uri)))       
            responses = await asyncio.gather(*tasks)
            print(connected)
            print("Number of connected cubes: "+str(len(connected)))
            print("Elapsed time for all responses: " + str(time.time()-startTime))
            return responses
        
def non_concurrent_get(firstTime:bool,uri:str,cube_list=None):
    global connected
    limits = httpx.Limits(max_keepalive_connections=None, max_connections=None)
    responses = []
    requestTimes =  []
    responseTimes = []
    if cube_list is not None:
        startTime = time.time()
        print("Contacting only :"+str(cube_list))
        for x in cube_list:
            requestTimes.append(datetime.now())
            responses.append(call_url_nc(x,uri))
            responseTimes.append(datetime.now())       
        print("Elapsed time for all responses: " + str(time.time()-startTime))
        return responses,requestTimes,responseTimes
    elif firstTime:
        startTime = time.time()
        for i in range(72,74):
            for j in range(0,256):
                if (i == 72 and j >=2) or i == 73:
                    requestTimes.append(datetime.now())
                    responses.append(call_url_nc(str(i)+"."+str(j),uri))
                    responseTimes.append(datetime.now())          
        print(connected)
        print("Number of connected cubes: "+str(len(connected)))
        print("Elapsed time for all responses: " + str(time.time()-startTime))
        return responses,requestTimes,responseTimes
    else:
        startTime = time.time()
        for x in connected:
            requestTimes.append(datetime.now())
            responses.append(call_url_nc(x,uri))
            responseTimes.append(datetime.now())    
        print(connected)
        print("Number of connected cubes: "+str(len(connected)))
        print("Elapsed time for all responses: " + str(time.time()-startTime))
        return responses,requestTimes,responseTimes

        
signal.signal(signal.SIGINT, handler)

def attempt_to_connect(attempts,expected,start_time):
    for i in range(attempts):
        requestTime = datetime.now()
        data = (asyncio.run(concurrent_get(firstTime=True,uri="state_guess")))
        #data,_,_ = non_concurrent_get(firstTime=True,uri="state_guess")
        if len(connected)>0:
            responseTime = datetime.now()
            experimentTime = time.time()-start_time
            dataTable = []
            errorTable = []
            for response in data:
                if type(response)==dict:
                    row = [experimentTime,requestTime,responseTime,response["fail_state"],response["macId"],response["mac0"],response["mac1"],response["mac2"],response["mac3"],response["mac4"],response["mac5"],response["mac6"],response["mac7"],response["update_num"],response["state_guess"],response["firmware"]]
                    dataTable.append(row)
                else:
                    errorTable.append(response)

        #print(dataTable)
        print("Error responses from ", errorTable)

        #with open(file_name, "ab") as f:
        #    numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')
        
        if len(connected)>=expected:
            break
        else:
            print("Not all expected cubes connected yet")
            #time.sleep() #There is enough timeout already with the connection timeout

    if len(connected) < expected:
        print("All "+ str(expected) +" cubes did not connect after "+str(attempts)+" attempts")
        exit()



async def do_ping(ipN):
    host = "192.168."+ipN
    #print(host)
    try:
        delay = await aioping.ping(host) *1000
        print("Ping response in "+str(delay)+" ms for "+ipN)
        return {"delay":delay,"ip":ipN}

    except TimeoutError:
        print("Timed out")


async def do_ping_test():
    #global connected
    connected = ["74.21","74.22","74.23","74.24","74.25","74.26","74.27","74.28"]
    tasks = []
    for x in connected:
        tasks.append(asyncio.create_task(do_ping(x)))       
    
    return_delays = await asyncio.gather(*tasks)

    return return_delays




  
def ping_Test(initial_connection_attempts = 10,expected=26,interval=0,file_name="ping_test_8"+str(time.time())+".csv"):
    
    start_time = time.time()

    #attempt_to_connect(initial_connection_attempts,expected,start_time=start_time)

    while(True):
        data = asyncio.run(do_ping_test())
        print(data)
        responseTime = datetime.now()
        dataTable = []
        for response in data:
            if response:
                row = [responseTime,response["delay"],response["ip"]]
                dataTable.append(row)

        with open(file_name, "ab") as f:
            #f.write(b"\n")
            numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')

        time.sleep(interval)



def main():
    ping_Test(initial_connection_attempts=10,expected=8,interval = 0)

if __name__ == "__main__":
    main()



