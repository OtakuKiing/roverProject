# Roadmap
> A quick index of each version, including subversions.

| Version | Name | Status | Summary |
|---------|------|--------|---------|
| v0.0 | Keyframe | Active | 2WD test rig, closed-loop PID |
| v0.1 | Crawler | Planned | ROS2 nodes, PlotJuggler |
| v1.0 | TBD | Planned | 4WD platform, sonar |
| v2.0 | TBD | Planned | RasPi, first SLAM attempt |
| vF | Wayfinder | Target | Full LiDAR/vision UGV |


## Progression Summary

```text
V0:
    Hardware:	      2WD, Differential steering, 3DP/Lasercut chassis
    Sensors:	      Wheel encoders, Buzzer, IMU
    Controllers:	  Arduino (C++)

V1:
    Hardware:	      4WD, Differential steering, COTS/Lasercut chassis
    Sensors:	      Wheel encoders, Buzzer, Sonar, LED screen
    Controllers:	  Arduino (C++), ESP32-S3

V2:
    Hardware:	      4WD, Differential steering, Aluminum chassis
    Sensors:	      Wheel encoders, Buzzer, Sonar, LED screen, Environmental sensor suite
    Controllers:	  Arduino (C++), ESP32-S3, RasPi 4B

...	etc

VF:
    Application:    Area mapping, surveillance, data collection, payload delivery, endurance missions,
    Hardware:       Aluminum chassis, AWD, AWS(?), Independent suspension 
    Sensors:        Odometry, LoRa telemetry, RTK GPS, LiDAR SLAM, Vision, Speakers, Lights
    Controllers:    Teensy 4.1 <-> ESP32 <-> RasPi, (micro)ROS2
    Modules:        [Stretch goals!] Robotic arm, Drone launcher, Solar 
```