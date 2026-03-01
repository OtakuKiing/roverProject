# TODO: split into functions - better grouping!
# minimise serial spam by sending key presses only on state change


# Example file
import pygame, serial, time, io

# pygame setup
pygame.init()
running = True

pygame.joystick.init()
joysticks = []

screen = pygame.display.set_mode((320, 200))
screen.fill((15, 15, 15))  # RGB fill
pygame.display.flip()  # update screen
pygame.display.set_caption("Input Window")

# serial setup
header = 0
heartbeat1 = 0
heartbeat2 = 0
awaiting_ACK = False
raw_Ser = serial.Serial(port='COM7', baudrate=115200)  # open serial port @ 115200 baud
ser = io.BufferedReader(raw_Ser, buffer_size=256)
print("Serial port: " + raw_Ser.name + "\nBaud: " + str(raw_Ser.baudrate))  # check which port and baud was really used

prev_Speed_L = 0
prev_Dir_L = 0
prev_Speed_R = 0
prev_Dir_R = 0

motor0_RPM = 0.0
motor1_RPM = 0.0


while running:
  # poll for events
  # pygame.QUIT event means the user clicked X to close your window
  for event in pygame.event.get():
    if event.type == pygame.QUIT:
      ser.close()  # close connection before killing
      running = False
    elif event.type == pygame.JOYDEVICEADDED:  # controller hotplugging handler
      print(f"New controller detected")
      joy = pygame.joystick.Joystick(event.device_index)
      joysticks.append(joy)

    ### joystick buttons ###
    if event.type == pygame.JOYBUTTONDOWN:
      match event.button:
        case 5:  # PS, kill motors
          packet = bytes([0xFF, 0x00])
          heartbeat1 = time.perf_counter()  # reset heartbeat timer
          
        case 11:  # D-pad up, led on
          packet = bytes([0xFF, 0x03, 0xFF])
          heartbeat1 = time.perf_counter()  # reset heartbeat timer

        case 12:  # D-pad down, led off
          packet = bytes([0xFF, 0x03, 0x00])
          heartbeat1 = time.perf_counter()  # reset heartbeat timer
        
        case _:
          packet = None
          
      raw_Ser.write(packet)


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
      packet = bytes([0xFF, 0x04, target_Dir_L, target_Speed_L])  # motor0, joystick map
      raw_Ser.write(packet)
      heartbeat1 = time.perf_counter()  # reset heartbeat timer
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
      packet = bytes([0xFF, 0x05, target_Dir_R, target_Speed_R])  # motor1, joystick map
      raw_Ser.write(packet)
      heartbeat1 = time.perf_counter()  # reset heartbeat timer
      prev_Speed_R = target_Speed_R
      prev_Dir_R = target_Dir_R
  

  ### keyboard controls ###
  
  keys = pygame.key.get_pressed()
  
  if keys[pygame.K_SPACE]:  # both motors, brake
    packet = bytes([0xFF, 0x04, 0x00, 0x00])
    raw_Ser.write(packet)
    packet = bytes([0xFF, 0x05, 0x00, 0x00])
    raw_Ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer


  if keys[pygame.K_PAGEUP]:  # motor0, forwards, full speed
    packet = bytes([0xFF, 0x04, 0x00, 0xFF])
    raw_Ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer
    
  if keys[pygame.K_LEFT]:  # motor0, forwards, half speed
    packet = bytes([0xFF, 0x04, 0x00, 0x80])
    raw_Ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer

  if keys[pygame.K_PAGEDOWN]:  # motor0, reverse, full speed
    packet = bytes([0xFF, 0x04, 0x01, 0xFF])
    raw_Ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer
    
  if keys[pygame.K_RIGHT]:  # motor0, reverse, half speed
    packet = bytes([0xFF, 0x04, 0x01, 0x80])
    raw_Ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer


	### serial control	###
  
  now = time.perf_counter()  # time now in ms
  
  if now - heartbeat1 > 2:  # send heartbeat after 2s with no command sent
    packet = bytes([0xFF, 0xFE])
    raw_Ser.write(packet)
    heartbeat1 = time.perf_counter()
    awaiting_ACK = True

        
  if raw_Ser.in_waiting:
    next_Byte = ser.peek(1)[:1]
    if((next_Byte == b'\xFF') and (header == 0)):  # check if new header is valid
      header = 1
      ser.read(1);  # eat header byte
      heartbeatStart = time.perf_counter();  # reset heartbeat timer

  if header == 1 and raw_Ser.in_waiting:  # check for valid header and waiting data
    next_Byte = ser.peek(1)[:1];  # check incoming packet id
    heartbeatStart = time.perf_counter();  # reset heartbeat timer if receiving packet, including invalid data
  
    match next_Byte:  # compare incoming packet id to commands
        
      case b'\x00':  # ACK kill motors
        ser.read(1)
        print("Motors killed!")

      case b'\x06':  # motor0 RPM value
        ser.read(1)
        motor0_RPM = ser.read(4)
        
      case b'\x07':  # motor1 RPM value
        ser.read(1)
        motor1_RPM = ser.read(4)
        
      case b'\xFE':  # ACK heartbeat
        ser.read(1)
        awaiting_ACK = False

      case _:
        ser.read(1)
        print(f"ERR, PARSE")
    
  if not header and raw_Ser.in_waiting: ser.read(1)

  if awaiting_ACK == True and now - heartbeat2 > 0.5:  # action to take when no ACK message recieved
    awaiting_ACK = True	 # placeholder
    


pygame.quit()