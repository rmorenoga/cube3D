import asyncio
import time
import httpx
import time
import numpy
from datetime import datetime
import signal
import random
import requests
import json

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

def attempt_to_connect(attempts,expected,start_time,file_name):
    for i in range(attempts):
        requestTime = datetime.now()
        data = (asyncio.run(concurrent_get(firstTime=True,uri="state_guess")))
        #data,_,_ = non_concurrent_get(firstTime=True,uri="state_guess")
        if len(connected)>0:
            responseTime = datetime.now()
            experimentTime = time.time()-start_time
            dataTable = []
            errorTable = []
            macTable = []
            for response in data:
                if type(response)==dict:
                    row = [experimentTime,requestTime,responseTime,response["fail_state"],response["macId"],response["mac0"],response["mac1"],response["mac2"],response["mac3"],response["mac4"],response["mac5"],response["mac6"],response["mac7"],response["update_num"],response["state_guess"],response["firmware"]]
                    dataTable.append(row)
                    if(response["macId"] not in macTable):
                        macTable.append(response["macId"])
                else:
                    errorTable.append(response)

        #print(dataTable)
        print("Error responses from ", errorTable)

        #with open(file_name, "ab") as f:
        #    numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')
        
        if len(connected)>=expected:
            return macTable
        else:
            print("Not all expected cubes connected yet")
            #time.sleep() #There is enough timeout already with the connection timeout

    if len(connected) < expected:
        print("All "+ str(expected) +" cubes did not connect after "+str(attempts)+" attempts")
        exit()

def select_failing_cubes(fail_percentage:float,macList):
    number_of_unique_elements = len(macList)*fail_percentage//100
    selected_cubes = random.sample(macList,number_of_unique_elements)
    print("Selected cubes for fail: "+str(selected_cubes))
    if len(selected_cubes)>0:
        return selected_cubes
    else:
        return None

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

# def change_failing_cubes_mode(failing_cubes,start_time,file_name):
#     if failing_cubes is not None:
#         requestTime = datetime.now()
#         data = (asyncio.run(concurrent_get(firstTime=True,uri="toggle_fail",cube_list=failing_cubes)))
#         #data,_,_ = non_concurrent_get(firstTime=False,uri="toggle_fail",cube_list=failing_cubes)
#         responseTime = datetime.now()
#         experimentTime = time.time()-start_time
#         dataTable = []
#         errorTable = []
#         for response in data:
#             if type(response)==dict:
#                 row = [experimentTime,requestTime,responseTime,response["fail_state"],response["update_num"],response["macId"]]
#                 dataTable.append(row)
#             else:
#                 errorTable.append(response)
#         print("Failing cubes responses")        
#         print("Error responses from ", errorTable)
        
#         with open("failing_cubes_"+file_name, "ab") as f:
#             numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')
#     else:
#         print("No cubes to put in failure mode")

def get_cube_status(file_name,start_time):
    requestTime = datetime.now()
    data = (asyncio.run(concurrent_get(firstTime=False,uri="state_guess")))
    #data,_,_ = non_concurrent_get(firstTime=False,uri="state_guess")
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

    print("Error responses from ", errorTable)

    #print(dataTable)

    with open(file_name, "ab") as f:
        #f.write(b"\n")
        numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')


def start_nca(start_time,file_name):
    global connected
    requestTime = datetime.now()
    data = (asyncio.run(concurrent_get(firstTime=False,uri="start-nca")))
    #data,_,_ = non_concurrent_get(firstTime=False,uri="start-nca")
    if len(connected)>0:
        responseTime = datetime.now()
        experimentTime = time.time()-start_time
        dataTable = []
        errorTable = []
        for response in data:
            if type(response)==dict:
                row = [experimentTime,requestTime,responseTime,response["runNCA"]]
                dataTable.append(row)
            else:
                errorTable.append(response)
        print("Error responses from ", errorTable)


        #print(dataTable)

        with open(file_name, "ab") as f:
            #f.write(b"\n")
            numpy.savetxt(f, dataTable,delimiter = ',',fmt ='% s')

def get_cube_root():
    
    data = (asyncio.run(concurrent_get(firstTime=False,uri="")))
    #data,_,_ = non_concurrent_get(firstTime=False,uri="state_guess")

    dataTable = []
    errorTable = []
    for response in data:
        if type(response)==dict:
            pass
        else:
            errorTable.append(response)

    print("Error responses from ", errorTable)

    #print(dataTable)


  
def failure_experiment(fail_percentage = 5,initial_connection_attempts = 10, file_name="failure_experiment"+str(time.time())+".csv",expected=26,mac_file_name="failing_cubes"+str(time.time())+".csv"):
    
    start_time = time.time()

    macTable = attempt_to_connect(initial_connection_attempts,expected,start_time=start_time,file_name=file_name)

    failing_cubes = select_failing_cubes(fail_percentage,macTable)

    print('Failing cubes', failing_cubes)

    with open(mac_file_name, "ab") as f:
        #f.write(b"\n")
        numpy.savetxt(f, failing_cubes,delimiter = ',',fmt ='% s')

    change_failing_cubes_mode(failing_cubes,start_time,file_name=file_name)

    # start_nca(start_time,start_file_name)
    
    # while(True):
    #     get_cube_status(file_name,start_time)
    #     #get_cube_root()
    #     #start_nca(start_time,start_file_name)
    #     time.sleep(5)



def main():
    fail_percentage = 15
    shapeClass = "Boat"
    shapeNumber = 144
    testNumber = 5
    expectedNCubes = 33
    failure_experiment(fail_percentage=fail_percentage,initial_connection_attempts=10,file_name="ShapeClass_responses_"+shapeClass+"_"+str(shapeNumber)+"_Test"+str(testNumber)+"_"+str(fail_percentage)+"%"+str(time.time())+".csv",expected=expectedNCubes,mac_file_name="failing_cubes_"+shapeClass+"_"+str(shapeNumber)+"_"+str(fail_percentage)+"%"+"_Test"+str(testNumber)+"_"+str(time.time())+".csv")

if __name__ == "__main__":
    main()



