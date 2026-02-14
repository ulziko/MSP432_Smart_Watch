# MSP432 Smart Watch  
>*Embedded Software for the Internet of Things — University of Trento, 2025*  
**Assoc. Prof. Yildirim Kasim Sinan**  
**Group 30:** Raffaelle Cella, Ulziikhishig Myagmarsuren, Namuunaa Boldbayar, Emuujin Myagmar  


## **Introduction**
This project implements a fully independent, low‑power smartwatch on the MSP432 LaunchPad and Educational BoosterPack MKII.  
The system provides essential daily functionalities such as time and date display, alarm management, a simple game, and an activity tracker based on accelerometer data.

Future improvements include adding memory for long‑term data storage and displaying user statistics over time.


## **Features**
- **Time & Date Display**  
- **Alarm Setting**  
- **Flappy Bird‑style Game** using the BoosterPack joystick  
- **Activity Tracker** with step counting and progress bar visualization  


## **Hardware**
- **MSP432P401R LaunchPad**
- <img width="640" height="360" alt="image" src="https://github.com/user-attachments/assets/b6aee0a4-1ea8-49de-83c9-2afcc729426a" />
- **Educational BoosterPack MKII**
- <img width="1420" height="798" alt="image" src="https://github.com/user-attachments/assets/c6a09722-b8ed-468f-9f78-e086633c634a" />


## **Getting Started**

### **Installation**
Clone the repository:
```bash
git clone https://github.com/ulziko/MSP432_Smart_Watch
```

### **Running the Project**
1. Open the project in **Code Composer Studio (CCS)**.  
2. Ensure the **SimpleLink MSP432P4 SDK** is installed and linked.  
3. Connect the MSP432 LaunchPad with the BoosterPack MKII attached.  
4. Build and flash the project onto the board.  

---

## **Project Structure**
```text
Smart Watch/
├── LcdDriver/                                   # LCD display driver module
│   ├── Crystalfontz128x128_ST7735.c             # Low-level LCD driver implementation
│   ├── Crystalfontz128x128_ST7735.h             # LCD driver header
│   ├── HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.c   # HAL layer for LCD
│   ├── HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h   # HAL header
│
├── include/                                     # Header files for application modules
│   ├── activity_tracker.h                       # Step detection interface
│   ├── alarm.h                                  # Alarm functionality
│   ├── game.h                                   # Game logic
│   ├── main_page.h                              # Main watch page
│   ├── menu.h                                   # Menu navigation
│   ├── system_time.h                            # System timekeeping
│   ├── tasks.h                                  # Task scheduler
│   ├── time_display.h                           # Time display UI
│
├── platform/                                    # Platform-specific system files
│   ├── msp432p401r.cmd                          # Linker command file
│   ├── startup_msp432p401r_ccs.c                # Startup code & interrupt vectors
│   ├── system_msp432p401r.c                     # System initialization
│   ├── system_time.c                            # System timekeeping implementation
│
├── src/                                         # Application source code
│   ├── activity_tracker.c                       # Step detection algorithm
│   ├── alarm.c                                  # Alarm logic
│   ├── game.c                                   # Game implementation
│   ├── images.c                                 # LCD image assets
│   ├── main_page.c                              # Main watch page logic
│   ├── tasks.c                                  # Task scheduler implementation
│   ├── time_display.c                           # Time display logic
│   └── main.c                                   # Main program entry point
│
├── .ccsproject                                  # CCS project metadata
├── .cproject                                    # CCS build configuration
├── .gitignore                                   # Git ignore rules
├── .project                                     # CCS project file
├── README.md                                    # Project documentation
```


## **Architecture**
The project is structured around modular C components, separating UI, sensor processing, system services, and application logic.  
A task‑based scheduler coordinates periodic updates, while interrupts handle real‑time events such as button presses and timer ticks.


## **Build Requirements**

### **Hardware**
- MSP432P401R LaunchPad  
- Educational BoosterPack MKII  
- Computer capable of flashing binaries  

### **Software**
- Code Composer Studio (CCS)  
- SimpleLink MSP432P4 SDK  
- Git (optional, for version control)


## **Development Notes**
When adding new tasks, it is recommended to register them inside the **handlers array** in `tasks.c` rather than defining new interrupt handlers.  
This avoids redefinition conflicts and keeps the system’s scheduling structure clean and maintainable.


## **Links**
- **Presentation**  
- **Demo Video**
