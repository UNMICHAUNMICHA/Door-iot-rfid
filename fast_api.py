from fastapi import FastAPI, Form, BackgroundTasks, Query, HTTPException
from fastapi.responses import JSONResponse
from datetime import datetime, timezone, timedelta
import mysql.connector
import random
import asyncio
from typing import Tuple
from pydantic import BaseModel
from typing import Optional
from fastapi.middleware.cors import CORSMiddleware
import time
app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["https://xxxxxxxxxxxxxxxxx"],  # สามารถแก้ไขเป็นโดเมนของคุณได้
    allow_credentials=True,
    allow_methods=["GET"],
    allow_headers=["*"],
)

# Configure your MySQL connection
mysql_config = {
    # "host": "localhost",
    # "user": "",
    # "password": "",
    # "database": "",
    # "autocommit": True  # เปิดใช้งาน autocommit
}

conn = mysql.connector.connect(**mysql_config)
cursor = conn.cursor()

def check_rfid_in_mysql(rfid_tag):
    query = "SELECT rfid, pin, user FROM esp32 WHERE rfid = %s"
    cursor.execute(query, (rfid_tag,))
    result = cursor.fetchone()
    
    if result:
        rfid, pin, user = result
        return True, user, pin
    else:
        return False, None, None

def unlock_response(user, pin):
    formatted_time = get_formatted_time()
    return JSONResponse(content={
        "status": "unlock",
        "user": user,
        "pin": pin,
        "timestamp": formatted_time
    })

def lock_response():
    formatted_time = get_formatted_time()
    return JSONResponse(content={
        "status": "lock",
        "timestamp": formatted_time
    })


@app.post("/rfid")
async def process_rfid(rfid_tag: str = Form(...)):
    print("RFID tag:", rfid_tag)
    
    is_valid, user, pin = check_rfid_in_mysql(rfid_tag)
    
    if is_valid:
        print("Name:", user)
        print("Id nisit:", pin)
        print("Door open")

        return unlock_response(user, pin)
    else:
        print("Door closed")
        return lock_response()

def get_formatted_time():
    local_time = datetime.now(timezone(timedelta(hours=7)))
    return local_time.strftime("%H:%M:%S%0f")[:-3]

def generate_otp():
    otp = ''
    for i in range(6):
        otp += str(random.randint(0, 9))
    return otp

def store_otp_in_mysql(otp):
    delete_query = "DELETE FROM otp"
    cursor.execute(delete_query)
    
    insert_query = "INSERT INTO otp (otp, timestamp) VALUES (%s, %s)"
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    cursor.execute(insert_query, (otp, timestamp))
    conn.commit()

async def send_otp_periodically():
    while True: 
        otp_generated = generate_otp()

        print("Generated OTP:", otp_generated)

        store_otp_in_mysql(otp_generated)

        await asyncio.sleep(120)

@app.on_event("startup")
async def startup_event():
    asyncio.create_task(send_otp_periodically())



@app.post("/otp")
async def otp(background_tasks: BackgroundTasks):
    background_tasks.add_task(send_otp_periodically)

    return {"message": "OTP generation task scheduled successfully"}





def check_code_in_database(code: str) -> Tuple[str, str, str]:

    query = "SELECT otp FROM otp ORDER BY timestamp DESC LIMIT 1"
    cursor.execute(query)
    result = cursor.fetchone()

    if result:
        latest_otp = result[0]
        print("Latest OTP from database:", latest_otp)

        if code == latest_otp:
            print("unlocking...")
            print(get_formatted_time())
            return "unlock", "unlock", latest_otp ,  
        else:
            print("locking...")
            print(get_formatted_time())
            return "lock", "lock", latest_otp
    else:
        print("No OTP found in the database, locking...")
        return "lock", "lock", None

@app.post("/check_code")
async def check_code(code: str = Form(...)):
    print(f"Received code: {code}")
    status, message, last_otp = check_code_in_database(code)
    time =  get_formatted_time()
    if status == "unlock":
        return {"status": "success", "message": "unlock", "last_otp": last_otp, "time": time }
    elif status == "lock":
        return {"status": "failure", "message": "lock", "last_otp": last_otp,  "time": time }
    else:
        print("Internal Server Error")
        raise HTTPException(status_code=500, detail="Internal Server Error")
        




stored_value = None  
last_update_time = None  

@app.get("/update_data/{value}")
async def update_data(value: int):
    global stored_value, last_update_time  

    try:
        print(f"Web to Door : {value}")
       
        stored_value = value
        last_update_time = time.time() 
        
        return {"message": "Data sent to ESP32 successfully"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to send data to ESP32: {e}")

@app.get("/get_otp_api")
async def get_stored_value():
    global stored_value, last_update_time
    
    if stored_value is not None and last_update_time is not None:
        if time.time() - last_update_time >= 10: 
            stored_value = 0  
            print("Door closer = 0")
           

    return {"stored_value": stored_value}
