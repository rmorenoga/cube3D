import os
from typing import Optional, List

from fastapi import FastAPI, Body, HTTPException, status, Request
from fastapi.responses import Response
from pydantic import ConfigDict, BaseModel, Field, EmailStr, BeforeValidator

from typing_extensions import Annotated

from bson import ObjectId
import motor.motor_asyncio
from pymongo import ReturnDocument

    

app = FastAPI(
    title="StatusReceiver",
    summary="An application to receive cube status",
)


#client = motor.motor_asyncio.AsyncIOMotorClient("localhost:27017")
client = motor.motor_asyncio.AsyncIOMotorClient("host.docker.internal:27017")
db = client.Test
status_collection = db.get_collection("Dummy")


# Represents an ObjectId field in the database.
# It will be represented as a `str` on the model so that it can be serialized to JSON.
PyObjectId = Annotated[str, BeforeValidator(str)]

class StatusModel(BaseModel):
    """
    Container for a complete status data entry.
    """

    # The primary key for the StudentModel, stored as a `str` on the instance.
    # This will be aliased to `_id` when sent to MongoDB,
    # but provided as `id` in the API requests and responses.
    id: Optional[PyObjectId] = Field(alias="_id", default=None)
    cell_updates: List[List]
    state_guesses: List
    update_times: List
    macId: int
    fail_state: bool
    firmware: str
    damage_mode: bool = False
    damage_class: int = 0


    model_config = ConfigDict(
        populate_by_name=True,
        arbitrary_types_allowed=True,
        json_schema_extra={
            "example": {
                "cell_updates": [[1,2,3],[2,4,9]],
                "state_guesses": [1,2,3,5],
                "update_times": [1,2,3],
                "macId": 16486315616546,
                "fail_state": False,
                "firmware": "2.57",
                "damage_mode": False,
                "damage_class":3

            }
        },
    )




@app.post(
    "/status_result",
    response_description="Add new staus",
    status_code=status.HTTP_201_CREATED,
    #status_code=status.HTTP_200_OK,
    response_model_by_alias=False,
)
# async def create_status(request: Request):

#     print(request.headers['content-type'])
#     print(request.headers['Content-Length'])
#     json_body = await request.json()
#     print(json_body)
async def create_status(status: StatusModel):
    """
    Insert a new student record.

    A unique `id` will be created and provided in the response.
    """
    new_status = await status_collection .insert_one(
         status.model_dump(by_alias=True, exclude=["id"])
    )
    created_status = await status_collection .find_one(
         {"_id": new_status.inserted_id}
    )
    return {"created": 1}

@app.get("/")
def get_root():
    return "Hi"
