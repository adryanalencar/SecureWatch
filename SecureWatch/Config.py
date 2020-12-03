import requests

url = "http://192.168.4.1/setMacAddress"


mac = int(0x0c)+int(0xec)+int(0x84)+int(0x0a)+int(0x1b)+int(0xd1)
print(mac)
payload = "-----011000010111000001101001\r\nContent-Disposition: form-data; name=\"mac\"\r\n\r\n123456\r\n-----011000010111000001101001--\r\n"
headers = {
    'Content-Type': "multipart/form-data",
    'content-type': "multipart/form-data; boundary=---011000010111000001101001"
    }

response = requests.request("POST", url, data={"mac1": int(0x0c), "mac2": int(0xec), "mac3": int(0x84), "mac4": int(0x0a), "mac5": int(0x1b), "mac6": int(0xd1)})

print(response.text)