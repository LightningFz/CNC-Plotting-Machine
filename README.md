# CNC Plotting Machine

This project is a **CNC Plotter Machine** designed to convert digital vector images into physical drawings. The machine automates the process of sketching and precision drawing, making it useful for art projects, PCB layouts, and other engineering tasks. This project involves a combination of hardware and software, with custom-built components and programming to control the movement of the plotter.

## Features

- **XY-Axis Movement**: The machine can move along the X and Y axes with high precision.
- **Pen Mechanism**: The plotter uses a servo motor to control the pen for raising and lowering during operation.
- **Custom G-Code Parser**: The software converts images or designs into G-code instructions that guide the plotter's movements.
- **Modular Design**: The system allows for easy adjustments and upgrades to improve accuracy or incorporate new tools.

## Components

1. **Hardware**:
   - Stepper motors (for X, Y axes movement)
   - Servo motor (for pen up/down movement)
   - Custom-built frame (wood or aluminum)
   - 3D printed parts for motor and pen mounts
   - Arduino-based control board (for stepper motor and servo control)

2. **Software**:
   - Custom G-code parser (converts images to G-code)
   - Motor control algorithms (drives the motors based on G-code instructions)
   - Calibration and fine-tuning routines to ensure high drawing accuracy.

## Setup Instructions

1. **Hardware Assembly**:
   - Build the CNC frame according to the provided design files.
   - Mount the stepper motors and servo for proper movement along the axes.
   - Attach the pen mechanism to the servo motor.

2. **Software Installation**:
   - Install the required Arduino libraries for motor and servo control.
   - Upload the provided code to the Arduino control board.
   - Run the calibration routine to ensure accurate movement.

3. **Running the Plotter**:
   - Convert your vector designs into G-code using the included script.
   - Upload the G-code to the machine and start the plotting process.

## Usage

1. **Design Creation**:
   - Use any vector graphics software (e.g., Inkscape) to create your design.
   - Convert the design to G-code using the provided G-code generator.

2. **Operation**:
   - Load the G-code onto the Arduino.
   - Start the plotter and watch as it draws the design with precision.

## Future Improvements

- Implement a Z-axis mechanism to allow for 3D printing or engraving capabilities.
- Add more sophisticated control features, such as wireless control or a touchscreen interface.

## Acknowledgments

Special thanks to the open-source CNC community for their resources and inspiration.
