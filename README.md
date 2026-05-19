# open5gs-xor-patch
open5gs-authen-xor-patch

Open5GS — XOR AKA Patch
สรุปการแก้ไข Source Code เพื่อรองรับ Test SIM (XOR Algorithm)
วันที่: 19 พฤษภาคม 2569  |  เวอร์ชัน: 1.0
1. ปัญหาที่พบ
Open5GS รองรับเฉพาะ Milenage Algorithm เป็นค่าเริ่มต้น ไม่รองรับ XOR Algorithm ที่ใช้ใน Test SIM cards (Anritsu, R&S, sysmoUSIM TS48) ส่งผลให้:
    • UE ที่ใช้ Test SIM แบบ XOR ไม่สามารถ Attach เข้าเครือข่ายได้
    • เกิด MAC Failure ในขั้นตอน Authentication
    • ไม่สามารถปิดการ Authentication ชั่วคราวใน LTE/5G ได้ (Mutual Auth บังคับทั้งสองฝั่ง)

2. ตัวเลือกที่พิจารณา
ตัวเลือก
รองรับ XOR
รองรับ 5G SA
หมายเหตุ
Patch Open5GS
✅ (patch นี้)
✅
แนะนำ — ใช้ทุก NF เดิม
srsEPC แทน Core
✅
❌
ไม่รองรับ 5G SA
free5GC
❌
✅
ไม่รองรับ XOR เช่นกัน
เปลี่ยน SIM ใหม่
✅
✅
แก้ที่ต้นเหตุ แต่ต้องมี SIM Programmer

3. แนวทางที่เลือก: Patch Open5GS
เขียน XOR Algorithm จาก 3GPP TS 34.108 Section 8 โดยตรง (ไม่ copy จาก srsRAN เพื่อหลีกปัญหา AGPL License) แล้วเพิ่มเข้าใน Open5GS source code

XOR Algorithm (3GPP TS 34.108)
Function
สูตรคำนวณ
ผลลัพธ์
f2 (RES)
K XOR RAND  [8 bytes แรก]
Response
f3 (CK)
rotate_left_1byte(K) XOR RAND
Cipher Key
f4 (IK)
rotate_right_1byte(K) XOR RAND
Integrity Key
f5 (AK)
K XOR RAND  [6 bytes แรก]
Anonymity Key
f1 (MAC-A)
(K XOR RAND) XOR (SQN||AMF||SQN||AMF) [8B]
Message Auth Code

4. ไฟล์ Source Code ที่แก้ไข
ไฟล์
NF ที่เกี่ยวข้อง
รายละเอียด
lib/crypt/ogs-kdf.c
ทุก NF (shared)
เพิ่ม ogs_xor_generate() และ ogs_xor_auc_sqn()
lib/crypt/ogs-kdf.h
ทุก NF (shared)
ประกาศ function prototype
lib/dbi/subscription.h/.c
UDR
เพิ่ม field sim_algo อ่านจาก MongoDB
src/udr/nudr-handler.c
UDR
ส่ง auth_method ที่ถูกต้องไปยัง UDM
src/udm/context.h
UDM
เพิ่ม sim_algo field ใน udm_ue_t struct
src/udm/nudr-handler.c
UDM
เรียก xor_generate() แทน milenage_generate()
src/udm/nudm-handler.c
UDM
เรียก xor_auc_sqn() ในกรณี Resync (AUTS)
webui/server/models/subscriber.js
WebUI
เพิ่ม field security.algo ใน MongoDB schema

5. ไฟล์ Patch ที่สร้าง
ชื่อไฟล์
เนื้อหา
0001-crypt-add-XOR-AKA-algorithm.patch
เพิ่ม XOR algorithm ใน lib/crypt/
0002-udm-wire-XOR-into-handlers.patch
เชื่อม XOR เข้า UDM handlers
0003-udr-dbi-add-sim-algo-field.patch
เพิ่ม sim_algo ใน UDR และ MongoDB
INSTALL-XOR-patch.md
คู่มือการติดตั้งและตั้งค่าแบบละเอียด

6. วิธี Apply Patch
6.1 Apply กับ Source Code
git clone https://github.com/open5gs/open5gs.git
cd open5gs
git apply 0001-crypt-add-XOR-AKA-algorithm.patch
git apply 0002-udm-wire-XOR-into-handlers.patch
git apply 0003-udr-dbi-add-sim-algo-field.patch

6.2 Build
meson build --prefix=/usr
ninja -C build
sudo ninja -C build install

7. การ Deploy บน Kubernetes + Helm
เนื่องจาก patch แก้ Source Code ต้อง Build Docker Image ใหม่ก่อน Deploy

7.1 Build Docker Image
docker build -t myregistry/open5gs-udm:xor-patch .
docker build -t myregistry/open5gs-udr:xor-patch .
docker push myregistry/open5gs-udm:xor-patch
docker push myregistry/open5gs-udr:xor-patch

7.2 NF ที่ต้อง Update Image
NF
ต้อง Build ใหม่?
เหตุผล
UDM
✅ ใช่
แก้ nudr-handler, nudm-handler, context.h
UDR
✅ ใช่
แก้ nudr-handler, dbi/subscription
AMF
❌ ไม่
ไม่มีการแก้ไข
AUSF
❌ ไม่
ไม่มีการแก้ไข
SMF, UPF, NRF, PCF, BSF
❌ ไม่
ไม่มีการแก้ไข

7.3 แก้ Helm values.yaml
udm:
  image:
    repository: myregistry/open5gs-udm
    tag: xor-patch

udr:
  image:
    repository: myregistry/open5gs-udr
    tag: xor-patch

7.4 Helm Upgrade
helm upgrade open5gs ./open5gs-helm -f values.yaml -n open5gs

8. การตั้งค่า Subscriber ใน MongoDB
เพิ่ม field security.algo = "XOR" ในเอกสาร subscriber ของ SIM ที่ต้องการ:

db.subscribers.updateOne(
  { imsi: "001010000000001" },
  { $set: {
      "security.k":    "465B5CE8B199B49FAA5F0A2EE238A6BC",
      "security.amf":  "8000",
      "security.algo": "XOR",
      "security.opc":  "00000000000000000000000000000000"  // dummy
  }}
)

⚠️  หมายเหตุ: field opc ใส่ dummy ได้ เพราะ XOR ไม่ใช้ค่านี้ในการคำนวณ

9. Backward Compatibility
    • Subscriber ที่ไม่มี security.algo หรือ algo ≠ "XOR" → ใช้ Milenage ตามปกติ
    • ไม่กระทบ Subscriber เดิมที่ติดตั้งไว้แล้ว
    • ไม่ต้องเปลี่ยน config ไฟล์ใดๆ ของ Open5GS

10. ข้อจำกัดที่ทราบ
    • รองรับเฉพาะ 5G SA — 4G MME path ยังไม่ได้ patch
    • EAP-AKA' ยังไม่รองรับ XOR (AUSF ไม่ได้แก้)
    • ใช้ OpenAPI_auth_method_NONE เป็น sentinel สำหรับ XOR (ชั่วคราว จนกว่า upstream จะเพิ่ม enum ใหม่)

11. ขั้นตอนถัดไป
#
งาน
สถานะ
1
ยืนยัน Helm chart ที่ใช้งาน (towards5gs / open5gs-helm / custom)
รอตรวจสอบ
2
ยืนยัน Container Registry ที่ใช้ (Docker Hub / Harbor / ECR)
รอตรวจสอบ
3
ยืนยันว่า Helm chart แยก image ต่อ NF หรือใช้ monolithic image
รอตรวจสอบ
4
Apply patch และ Build image ใน environment จริง
รอดำเนินการ
5
ทดสอบ UE attach ด้วย Test SIM XOR
รอดำเนินการ
