import asyncio
import time
import httpx
import time
import numpy
import signal
from datetime import datetime
import json

connected = []

def handler(signum, frame):
    res = input("Ctrl-c was pressed. Do you really want to exit? y/n ")
    if res == 'y':
        exit(1)

signal.signal(signal.SIGINT, handler)

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
    except Exception as e:
        print(f"ConnectError {url}")


async def concurrent_get(uri:str,server_IP):
    global connected
    limits = httpx.Limits(max_keepalive_connections=None, max_connections=None)
    tasks = []
    async with httpx.AsyncClient(limits = limits) as session:  #use httpx
        startTime = time.time()
        for i in range(72,74):
            for j in range(0,256):
                if (i == 72 and (j >=2 and j!= server_IP)) or (i == 73 and j < 255):
                    tasks.append(asyncio.create_task(call_url_get(session,str(i)+"."+str(j),uri)))          
        responses = await asyncio.gather(*tasks)
        print(connected)
        print("Number of connected cubes: "+str(len(connected)))
        print("Elapsed time for all responses: " + str(time.time()-startTime))
        return responses

async def concurrent_post(uri:str,json_body):
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
    
def attempt_to_connect(attempts,expected,start_time,server_IP):
    for i in range(attempts):
        requestTime = datetime.now()
        #asyncio.run(concurrent_get(uri="start-nca"))
        data = (asyncio.run(concurrent_get(uri="state_guess",server_IP=server_IP)))
        if len(connected)>0:
            responseTime = datetime.now()
            experimentTime = time.time()-start_time
            dataTable = []
            for response in data:
                if response:
                    row = [experimentTime,requestTime,responseTime,response["fail_state"],response["macId"],response["mac0"],response["mac1"],response["mac2"],response["mac3"],response["mac4"],response["mac5"],response["mac6"],response["mac7"],response["update_num"],response["state_guess"],response["firmware"],response["damage_mode"],response["damage_class"]]
                    dataTable.append(row)

        #print(dataTable)
        
        if len(connected)>=expected:
            break
        else:
            print("Not all expected cubes connected yet")
            #time.sleep() #There is enough timeout already with the connection timeout

    if len(connected) < expected:
        print("All "+ str(expected) +" cubes did not connect after "+str(attempts)+" attempts")
        exit()

def set_damage_mode(damage_class, start_time, file_name):
    requestTime = datetime.now()
    json_request = json.dumps({'damage_class': damage_class})
    print("Request json body: ", json_request)
    data = (asyncio.run(concurrent_post(uri="damage_mode",json_body=json_request)))
    if len(connected)>0:
        responseTime = datetime.now()
        experimentTime = time.time()-start_time
        dataTable = []
        errorTable = []
        for response in data:
            if type(response)==dict:
                row = [experimentTime,requestTime,responseTime,response["damage_mode"],response["damage_class"]]
                dataTable.append(row)
            else:
                errorTable.append(response)
        print("Error responses from ", errorTable)

        with open(file_name, "ab") as f:
            #f.write(b"\n")
            numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')
    else:
        print("No cubes to put into damage mode")

def setDamageMode(damage_class = 0,expected=26,initial_connection_attempts=10,file_name="setDamageResponses"+str(time.time())+".csv",server_IP=21):

    start_time = time.time()

    attempt_to_connect(initial_connection_attempts,expected,start_time=start_time,server_IP=server_IP)

    set_damage_mode(damage_class,start_time,file_name)

    print("Finished setting damage mode in all modules")

def main():
    setDamageMode(damage_class=3,expected=197,initial_connection_attempts=10,file_name="setDamageResponses"+str(time.time())+".csv",server_IP=84)

if __name__ == "__main__":
    main()
