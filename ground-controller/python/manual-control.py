# TODO: 
# split into nicer functions - better grouping!
# test controller inputs

'''
| Direction     | Category  | Message ID | Content                                           |
| ------------- | --------- | ---------- | ------------------------------------------------- |
| Pi -> Arduino | Admin     | 0x00       | Emergency state X - run routine and await command |
|               |           | 0x01       | Exit emergency state                              |
|               |           | 0x02       | Toggle verbose telemetry                          |
|               |           | 0x03       | Toggle audio alerts                               |
|               |           | 0x04       | Debug 1 (eg live PID)                             |
|               |           | 0x05       | Debug 2 (eg live PID)                             |
|               | Command   | 0x10       | Move motors                                       |
|---------------|-----------|------------|---------------------------------------------------|
| Arduino -> Pi | Admin     | 0x80       | Error (eg missing expected component)             |
|               | Telemetry | 0x90       | Motors RPM                                        |
|               |           | 0x91       | Motors voltage                                    |
|               |           | 0x92       | Motors current                                    |
|               |           | 0x93       | Encoder counts (verbose)                          |
|               |           | 0xA0       | Battery voltage and current                       |
|               |           | 0xA1       | Battery temperature                               |
|               |           | 0xB0       | Raw IMU                                           |
|               |           | 0xB1       | Raw GNSS                                          |
|---------------|-----------|------------|---------------------------------------------------|
| Bidirectional | Admin     | 0xFE       | Heartbeat                                         |
'''

# Example file
import pygame, serial, time, queue, struct, threading
import serial.tools.list_ports

# pygame setup
pygame.init()
running = True

pygame.joystick.init()
joysticks = []

screen = pygame.display.set_mode((320, 200))
screen.fill((15, 15, 15))  # RGB fill
pygame.display.flip()  # update screen
pygame.display.set_caption("Input Window")

### serial setup ###
serial_thread = None
serial_Queue = queue.Queue()
prev_Packet_Out = None
header = 0
heartbeat1 = 0
awaiting_ACK = False

raw_Ser: serial.Serial | None = None

while raw_Ser == None:  # search for arduino
  all_Ports = serial.tools.list_ports.comports()	# get all open serial ports
  for comport in all_Ports:
    if "arduino" in comport.description.lower():
      raw_Ser = serial.Serial(port=comport.device, baudrate=115200, timeout=1)  # open serial port @ 115200 baud
      print(f"Serial port: {raw_Ser.name or "unknown"}\nBaud: {raw_Ser.baudrate}")  # print which port and baud was really used
      break

  if raw_Ser == None: print("No port available")

## motor data input ##
prev_Speed_L = 0
prev_Dir_L = 0
prev_Speed_R = 0
prev_Dir_R = 0

motor0_RPM = 0.0
motor1_RPM = 0.0

def serialSendUrgent(packet_Out):  # drain the queue
    while not serial_Queue.empty():
        try: serial_Queue.get_nowait()
        except queue.Empty: break
    serial_Queue.put(packet_Out)

def serialIO():
  global running, motor0_RPM, motor1_RPM, heartbeat1, awaiting_ACK  # allows these variables to be modified globally
  header = 0
  packet_Out = b''
  while running:
    ### serial control	###
    assert raw_Ser is not None

    ## heartbeat ##
    now = time.perf_counter()  # time now in ms
    if now - heartbeat1 > 0.5:  # send heartbeat after 0.5s with no command sent
      raw_Ser.write(b'\xFF\xFE')
      heartbeat1 = time.perf_counter()
      awaiting_ACK = True

    ## write serial ##
    try:
      packet_Out = serial_Queue.get_nowait()
      if packet_Out is not None:
        raw_Ser.write(packet_Out)
        print("Sent this:", packet_Out)
        heartbeat1 = time.perf_counter()
      else: break   # sentinel command, exit
    except queue.Empty:
      pass

    ## read serial ##
    if raw_Ser.in_waiting:
      byte = raw_Ser.read(1)
    else: byte = b''
      
    if byte == b'\xFF' and header == 0:  # resets loop, don't send header to match-case
      header = 1  
      continue

    if header == 1:  # match packet structure to command database
      match byte:
        case b'\x01':  # motor0 data
          header = 0
          packet_In = raw_Ser.read(4)
          if len(packet_In) == 4:  # check if packet is valid
            motor0_RPM = struct.unpack('<f', packet_In)[0]  # convert bytes from array to little-endian float
            print("Recieved from motor0:", motor0_RPM)
          else:
            print("Bad packet! motor0")

        case b'\x02':  # motor1 data
          header = 0
          packet_In = raw_Ser.read(4)
          if len(packet_In) == 4:  # check if packet is valid
            motor1_RPM = struct.unpack('<f', packet_In)[0]  # convert bytes from array to little-endian float
          else: 
            print("Bad packet! motor1")
        
        case b'\xFE':  # heartbeat ACK
          awaiting_ACK = False
          header = 0

        case b'\x00':  # e-stop active
          print("E-stop active!")
          header = 0

        case _:  # ignore bad data
          header = 0

try:
  serial_thread = threading.Thread(target=serialIO, daemon=True)
  serial_thread.start()  # start threaded serial 

  while running:
    # poll for events
    for event in pygame.event.get():
      if event.type == pygame.QUIT:  # window closed
        running = False

      elif event.type == pygame.JOYDEVICEADDED:  # controller hotplugging handler
        print(f"New controller detected")
        joy = pygame.joystick.Joystick(event.device_index)
        joysticks.append(joy)

      ### controller buttons ###
      elif event.type == pygame.JOYBUTTONDOWN:
        match event.button:
          case 5:  # PS, kill motors
            serialSendUrgent(b'\xFF\x00\x00\x00\x00\x00')
            
          case 11:  # D-pad up
            serial_Queue.put_nowait(b'\xFF\x03\xFF\x00\x00\x00')

          case 12:  # D-pad down
            serial_Queue.put_nowait(b'\xFF\x03\x00\x00\x00\x00')

      ### keyboard controls ###
      elif event.type == pygame.KEYDOWN:  # send command once on key down
        match event.key:
          case pygame.K_w:  # fwd full
            serial_Queue.put_nowait(b'\xFF\x01\x00\xFF\x00\x00')
            serial_Queue.put_nowait(b'\xFF\x02\x00\xFF\x00\x00')

          case pygame.K_s:  # rev full
            serial_Queue.put_nowait(b'\xFF\x01\x01\xFF\x00\x00')
            serial_Queue.put_nowait(b'\xFF\x02\x01\xFF\x00\x00')

          case pygame.K_a:  # sweep left
            serial_Queue.put_nowait(b'\xFF\x01\x00\x40\x00\x00')
            serial_Queue.put_nowait(b'\xFF\x02\x00\xFF\x00\x00')

          case pygame.K_d:  # sweep right
            serial_Queue.put_nowait(b'\xFF\x01\x00\xFF\x00\x00')
            serial_Queue.put_nowait(b'\xFF\x02\x00\x40\x00\x00')

          case pygame.K_q:  # pivot left
            serial_Queue.put_nowait(b'\xFF\x01\x01\xFF\x00\x00')
            serial_Queue.put_nowait(b'\xFF\x02\x00\xFF\x00\x00')

          case pygame.K_e:  # pivot right
            serial_Queue.put_nowait(b'\xFF\x01\x00\xFF\x00\x00')
            serial_Queue.put_nowait(b'\xFF\x02\x01\xFF\x00\x00')

      elif event.type == pygame.KEYUP:  # send stop command on any key up
        serial_Queue.put_nowait(b'\xFF\x01\x00\x00\x00\x00')
        serial_Queue.put_nowait(b'\xFF\x02\x00\x00\x00\x00')


    ###	joysticks	###
    
    for joystick in joysticks:
      leftStick = joystick.get_axis(1)  # left stick y
      rightStick = joystick.get_axis(3)  # right stick y

      ## left stick ##
      if abs(leftStick) > 0.03:  # ignore deadspace
        target_Speed_L = round(abs(leftStick * 255))  # map joystick to pwm
        target_Dir_L = 0 if leftStick < 0 else 1  # negative value means reverse direction
      else:
        target_Speed_L = 0
        target_Dir_L = 0

      if (prev_Speed_L != target_Speed_L) or (prev_Dir_L != target_Dir_L):  # only update on change
        serial_Queue.put_nowait(bytes([0xFF, 0x01, target_Dir_L, target_Speed_L]))  # motor0, joystick map
        prev_Speed_L = target_Speed_L
        prev_Dir_L = target_Dir_L
      
      ## right stick ##
      if abs(rightStick) > 0.03:  # ignore deadspace
        target_Speed_R = round(abs(rightStick * 255))  # map joystick to pwm
        target_Dir_R = 0 if rightStick < 0 else 1  # negative value means reverse direction
      else:
        target_Speed_R = 0
        target_Dir_R = 0

      if (prev_Speed_R != target_Speed_R) or (prev_Dir_R != target_Dir_R):  # only update on change
        serial_Queue.put_nowait(bytes([0xFF, 0x02, target_Dir_R, target_Speed_R]))  # motor1, joystick map
        prev_Speed_R = target_Speed_R
        prev_Dir_R = target_Dir_R
    
finally:

  serial_Queue.put(None)
  raw_Ser.close()

  try:  # make sure thread is running before terminating
    assert serial_thread is not None
    serial_thread.join()
  except AssertionError:
    print("Serial thread is not running!")

  print("Closing safely...")
  pygame.quit()