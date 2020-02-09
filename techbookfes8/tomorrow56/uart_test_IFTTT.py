from fpioa_manager import fm
from machine import UART
from time import sleep

fm.register(35, fm.fpioa.UART2_TX, force=True)
fm.register(34, fm.fpioa.UART2_RX, force=True)

uart_Port = UART(UART.UART2, 115200,8,None,0, timeout=1000, read_buf_len= 4096)

SSID = 'xxxxxxxxxxxx' # wifi ssid here
PASS = 'xxxxxxxxxxxx' # wifi key

# IFTTT Trigger Setting
IFTTT_KEY = "xxxxxxxxxxxxxxxxxxxxxx"
EVENT_NAME = "MyM5StickVEvent"

HOST = "maker.ifttt.com"
PORT = "80"
uri = "/trigger/" + EVENT_NAME + "/with/key/" + IFTTT_KEY


echo_read = True
echo_write = False

def read_print(read_str):
    if echo_read:
#        print(read_str.replace('\r\n', ''))
        print(read_str.replace('\r\n', '\n'))

def sendCheckReply(command, reply):
    result = False

    write_str = command + '\r\n'
    uart_Port.write(write_str)
    if echo_write:
        print(write_str)

    count = 1000    # timeout 10 sec
    while count > 0:
        read_data = uart_Port.read()
        if read_data is not None:
            read_str = read_data.decode('utf-8', 'ignore')
            read_print(read_str)
            if reply in read_str:
                result = True
                break
        count = count - 1
        sleep(0.01)
    return result

def sendCheckReplyConnect(command, reply, timeout):
    write_str = command + '\r\n'
    uart_Port.write(write_str)
    if echo_write:
        print(write_str)

    count = timeout / 10

    while count > 0:
        read_data = uart_Port.read()
        if read_data is not None:
            if b'AT+CWJAP' in read_data or b'WIFI CONNECTED' in read_data or b'GOT IP' in read_data:
                read_str = read_data.decode('utf-8', 'ignore')
                read_print(read_str)
                if reply in read_str:
                    break
        count = count - 1
        sleep(0.01)

    return True

def espReset():
    if sendCheckReply('AT+RST', 'OK') != True:
        return False

    if sendCheckReply('ATE1', 'OK') != True:
        return False

    return True

def getVersion():
    if sendCheckReply('AT+GMR', 'OK') != True:
        return False

    return True;

def ESPconnectAP(SSID, PASS):
    if sendCheckReply('AT+CWMODE=1', 'OK') != True:
        return False

    connectStr = 'AT+CWJAP=\"'
    connectStr += SSID
    connectStr += '\",\"'
    connectStr += PASS
    connectStr += '\"'
    sendCheckReplyConnect(connectStr, 'GOT IP', 5000)

    return True;

def setupWiFi():
    print("Soft resetting...")
    if espReset() != True:
        print("** Resst fail! **")
        return False

    print("Checking for ESP AT response")
    if sendCheckReply('AT', 'OK') != True:
        print("** AT response fail! **")
        return False

    getVersion()

    print("Connecting to " + SSID)
    ESPconnectAP(SSID, PASS)

    print("Single Client Mode")
    if sendCheckReply("AT+CIPMUX=0", "OK") != True:
        return False

    return True

def getIP():
    if sendCheckReply('AT+CIFSR', 'OK') != True:
        return False

    return True;

def disconnectTCP():
    if sendCheckReply('AT+CIPCLOSE', 'OK') != True:
        return False

    return True;

def SENDTrigger(host, port, uri, value1, value2, value3):
    # "value*" format must be a string 
    cmdStr = 'AT+CIPSTART=\"TCP\",\"'
    cmdStr += host
    cmdStr += '\",'
    cmdStr += port
    sendCheckReply(cmdStr, 'OK')

    # Construct our data with JSON
    content = '{\"value1\":\"' + value1 + '\",\"value2\":\"' + value2 + '\",\"value3\":\"' + value3 + '\"}'

    # Construct our HTTP call
    httpPacket = "POST " + uri + " HTTP/1.1\r\nHost: " + host + "\r\nContent-Length: " + str(len(content)) + " \r\nContent-Type: application/json\r\n\r\n" + content +"\r\n"
  
    cmdStr = 'AT+CIPSEND=' + str(len(httpPacket))
    sendCheckReply(cmdStr, '>')

    cmdStr = httpPacket
    sendCheckReply(cmdStr, 'SEND OK')

    count = 1

    while count > 0:
        read_data = uart_Port.read()
        if read_data is not None:
            read_str = read_data.decode('utf-8', 'ignore')
            read_print(read_str)
        count = count - 1
        sleep(0.01)

# *******************************
# * Main                        *
# *******************************
setupWiFi()
getIP()

data1 = 'Clapper(white)'
data2 = 'Clapper(pink)'
data3 = 'Mr.Tall(red)'

SENDTrigger(HOST, PORT, uri, data1, data2, data3)

disconnectTCP()
