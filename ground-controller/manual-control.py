# TODO: split into functions - better grouping!
# minimise serial spam by sending key presses only on state change


# Example file
import pygame, serial, time

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
heartbeat1 = 0
heartbeat2 = 0
awaiting_ACK = False
ser = serial.Serial(port='COM4', baudrate=115200)  # open serial port @ 115200 baud
print("Serial port: " + ser.name + "\nBaud: " + str(ser.baudrate))  # check which port and baud was really used

prev_Speed_L = 0
prev_Dir_L = 0
prev_Speed_R = 0
prev_Dir_R = 0


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
          
      ser.write(packet)


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
      ser.write(packet)
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
      ser.write(packet)
      heartbeat1 = time.perf_counter()  # reset heartbeat timer
      prev_Speed_R = target_Speed_R
      prev_Dir_R = target_Dir_R
  

  ### keyboard controls ###
  
  keys = pygame.key.get_pressed()
  
  if keys[pygame.K_SPACE]:  # both motors, brake
    packet = bytes([0xFF, 0x04, 0x00, 0x00])
    ser.write(packet)
    packet = bytes([0xFF, 0x05, 0x00, 0x00])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer


  if keys[pygame.K_q]:  # motor0, forwards, full speed
    packet = bytes([0xFF, 0x04, 0x01, 0xFF])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer
    
  if keys[pygame.K_a]:  # motor0, reverse, full speed
    packet = bytes([0xFF, 0x04, 0x00, 0xFF])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer

  if keys[pygame.K_w]:  # motor0, brake
    packet = bytes([0xFF, 0x04, 0x00, 0x00])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer
    
    
  if keys[pygame.K_e]:  # motor1, forwards, full speed
    packet = bytes([0xFF, 0x05, 0x01, 0xFF])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer
    
  if keys[pygame.K_d]:  # motor1, reverse, full speed
    packet = bytes([0xFF, 0x05, 0x00, 0xFF])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer
 
  if keys[pygame.K_s]:  # motor1, brake
    packet = bytes([0xFF, 0x05, 0x00, 0x00])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer
    

  if keys[pygame.K_z]:  # led, on
    packet = bytes([0xFF, 0x03, 0xFF])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer

  if keys[pygame.K_x]:  # led, off
    packet = bytes([0xFF, 0x03, 0x00])
    ser.write(packet)
    heartbeat1 = time.perf_counter()  # reset heartbeat timer


	### heartbeat	###
  
  now = time.perf_counter()  # time now in ms
  
  if now - heartbeat1 > 0.5:  # send heartbeat after 0.5s with no command sent
    packet = bytes([0xFF, 0xFE])
    ser.write(packet)
    heartbeat1 = time.perf_counter()
    awaiting_ACK = True


  if ser.in_waiting > 0:  # listen for ACK heartbeat message
    ack_Msg = ser.read()
    if ack_Msg == b'\xFF':
      heartbeat2 = time.perf_counter()
      awaiting_ACK = False
    else:
      print(f"ERR, PARSE")

  if awaiting_ACK == True and now - heartbeat2 > 0.5:
    # action to take when no ACK message recieved
    awaiting_ACK = True	 # placeholder
    


pygame.quit()