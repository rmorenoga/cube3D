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
    except Exception as e:
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
    for i in range(attempts):
        requestTime = datetime.now()
        #asyncio.run(concurrent_get(uri="start-nca"))
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

        with open(file_name, "ab") as f:
            numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')
        
        if len(connected)>=expected:
            break
        else:
            print("Not all expected cubes connected yet")
            #time.sleep() #There is enough timeout already with the connection timeout

    if len(connected) < expected:
        print("All "+ str(expected) +" cubes did not connect after "+str(attempts)+" attempts")
        exit()


def change_failing_cubes_mode(cubes_macs,start_time,file_name):
    if cubes_macs is not None:
        requestTime = datetime.now()
        json_request = json.dumps({'macs':cubes_macs})
        print("Request json body: ", json_request)
        data = (asyncio.run(concurrent_post(uri="mac_search",json_body=json_request)))
        responseTime = datetime.now()
        experimentTime = time.time()-start_time
        dataTable = []
        for response in data:
            if response:
                row = [experimentTime,requestTime,responseTime,response["fail_state"],response["update_num"]]
                dataTable.append(row)
        print("Failing cubes responses")        
        print(dataTable)

        check = 0
        for response in dataTable:
            if response[3] == True:
                check +=1

        print("Number of cubes that changed state and responded: ", check)
        
        with open("locate_failing_cubes_responses"+file_name, "ab") as f:
            numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')
    else:
        print("No cubes to put in failure mode")

def clear_fail_flag(start_time,file_name):
    global connected
    requestTime = datetime.now()
    data = (asyncio.run(concurrent_get(uri="clear_fail")))
    #data,_,_ = non_concurrent_get(firstTime=False,uri="start-nca")
    if len(connected)>0:
        responseTime = datetime.now()
        experimentTime = time.time()-start_time
        dataTable = []
        errorTable = []
        for response in data:
            if type(response)==dict:
                row = [experimentTime,requestTime,responseTime,response["fail_state"],response["macId"]]
                dataTable.append(row)
            else:
                errorTable.append(response)
        print("Error responses from ", errorTable)


        #print(dataTable)

        with open(file_name, "ab") as f:
            #f.write(b"\n")
            numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')


def get_cubes_macs(filename_macs): 
    df = pd.read_csv(filename_macs,header=0,names=["MAC"]) 
    cubes_macs = df["MAC"].tolist()
    print(df)
    print(cubes_macs) 
    return cubes_macs

def locate_failing_cubes(expected=26,initial_connection_attempts=10,file_name="locate_failing_responses"+str(time.time())+".csv",filename_macs="cubes_macs.csv"):

    start_time = time.time()

    attempt_to_connect(initial_connection_attempts,expected,start_time=start_time,file_name=file_name)

    clear_fail_flag(start_time,file_name)

    cubes_macs = get_cubes_macs(filename_macs)

    change_failing_cubes_mode(cubes_macs,start_time,file_name=file_name)

    print("Finished changing all failing modules mode")



def main():
    locate_failing_cubes(expected=26,initial_connection_attempts=10,file_name="locate_failing_responses_Table_372"+str(time.time())+".csv",filename_macs="locateCubeMac.csv")

if __name__ == "__main__":
    main()