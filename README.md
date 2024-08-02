# โปรเจค ชิ้นงานจบปี1 
ของผมเป็นผลงานและได้ศึกษาโค๊ดเพื่อพัฒนาระบบ
rfid เชื่อม mysql และระบบ otp



# การทำงาน โปรแกรม
คือ rfid อ่านค่าจากบัตรจะทำการส่งค่าไปที่ api เพื่อไปเช็คที่ database ว่าตรงหรือป่าวเพื่อให้ประตูเปิดหรือปิด 	และมีระบบ otp ที่จะสุ่มเลข 6 หลัก เพื่อกรอกที่หน้าประตูได้เช่นกัน	
และสามารถดูหลังบ้านได้ว่ามีใครเข้ามาในห้องบ้าง

# ผู้ดูแลระบบ 
![enter image description here](https://github.com/UNMICHAUNMICHA/Door-iot-rfid/blob/main/Screenshot%202024-08-02%20132434.png?raw=true)

![enter image description here](https://github.com/UNMICHAUNMICHA/Door-iot-rfid/blob/main/Screenshot%202024-08-02%20132444.png?raw=true)
สามารถ เพิ่มลบแก้ไข ได้และเปิด/ปิด บัตรแต่ละใบได้
otp สุ่มทุกครั้งที่มีการใช้ไปแล้ว และจะสุ่มใหม่ทุก1 นาที
และมีปุ่มเปิดประตูจากหน้าเว็บได้เลยสำหรับผู้ดูเเล

# Log
![enter image description here](https://github.com/UNMICHAUNMICHA/Door-iot-rfid/blob/main/Screenshot%202024-08-02%20132858.png?raw=true)
เช็คได้ว่าใครเข้ามา 


# Tool 
	- Python fast api
	- C arduino
	- Mysql
