# TODO: split into functions - better grouping!


# Example file
import pygame, serial, time, queue, struct, threading

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
serial_Queue = queue.Queue()
prev_Packet_Out = None
header = 0
heartbeat1 = 0
awaiting_ACK = False
raw_Ser = serial.Serial(port='COM4', baudrate=115200, timeout=1)  # open serial port @ 115200 baud
print("Serial port: " + raw_Ser.name + "\nBaud: " + str(raw_Ser.baudrate))  # check which port and baud was really used

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
      
    if byte == b'\xFF' and header == 0: 
      header = 1  
      continue  # resets loop, don't send header to match-case
    if header == 1:
      match byte:
        case b'\x04':  # motor0 data
          header = 0
          packet_In = raw_Ser.read(4)
          if len(packet_In) == 4:  # check if packet is valid
            motor0_RPM = struct.unpack('<f', packet_In)[0]  # convert bytes from array to little-endian float
            print(motor0_RPM)
          else:
            print("Bad packet! motor0")

        case b'\x05':  # motor1 data
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
            serialSendUrgent(b'\xFF\x00')
            
          case 11:  # D-pad up
            serial_Queue.put_nowait(b'\xFF\x03\xFF')

          case 12:  # D-pad down
            serial_Queue.put_nowait(b'\xFF\x03\x00')

      ### keyboard controls ###
      elif event.type == pygame.KEYDOWN:  # send command once on key down
        match event.key:
          case pygame.K_PAGEUP:  # motor0 fwd full
            serial_Queue.put_nowait(b'\xFF\x04\x00\xFF')

          case pygame.K_LEFT:  # motor0 fwd half
            serial_Queue.put_nowait(b'\xFF\x04\x00\x80')

          case pygame.K_PAGEDOWN:  # motor1 rev full
            serial_Queue.put_nowait(b'\xFF\x05\x01\xFF')

          case pygame.K_RIGHT:  # motor1 rev half
            serial_Queue.put_nowait(b'\xFF\x05\x01\x80')

      elif event.type == pygame.KEYUP:
        if event.key in (pygame.K_PAGEUP, pygame.K_LEFT):  # motor0, brk
            serial_Queue.put_nowait(b'\xFF\x04\x00\x00')
        elif event.key in (pygame.K_PAGEDOWN, pygame.K_RIGHT):  # motor1, brk
            serial_Queue.put_nowait(b'\xFF\x05\x00\x00')
            serial_Queue.put_nowait(b'\xFF\x05\x00\x00')


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
        serial_Queue.put_nowait(bytes([0xFF, 0x04, target_Dir_L, target_Speed_L]))  # motor1, joystick map
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
        serial_Queue.put_nowait(bytes([0xFF, 0x05, target_Dir_R, target_Speed_R]))  # motor1, joystick map
        prev_Speed_R = target_Speed_R
        prev_Dir_R = target_Dir_R
    

finally:
  serial_Queue.put(None)
  serial_thread.join()
  raw_Ser.close()
  print("Closing safely...")
  pygame.quit()