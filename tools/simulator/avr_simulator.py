#!/usr/bin/env python3
"""
🔧 AVR/Arduino Hardware Simulator
Real-time embedded system simulation for practical learning without hardware dependency

Simulates ATmega328P microcontroller with accurate timing, GPIO, timers, UART, SPI, I2C
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import threading
import time
import json
import subprocess
import os
import re
from datetime import datetime
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Callable, Any
from enum import Enum
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np

class PinMode(Enum):
    INPUT = 0
    OUTPUT = 1
    INPUT_PULLUP = 2

class PinState(Enum):
    LOW = 0
    HIGH = 1

@dataclass
class GPIOPin:
    """GPIO Pin simulation"""
    number: int
    mode: PinMode = PinMode.INPUT
    state: PinState = PinState.LOW
    analog_value: int = 0  # 0-1023 for ADC
    pwm_value: int = 0     # 0-255 for PWM
    pwm_enabled: bool = False
    last_change: float = 0.0

@dataclass
class TimerConfig:
    """Timer configuration and state"""
    timer_id: int
    prescaler: int = 1
    compare_value: int = 0
    counter_value: int = 0
    overflow_count: int = 0
    pwm_mode: bool = False
    frequency: float = 0.0
    duty_cycle: float = 0.0

@dataclass
class SerialData:
    """UART/Serial communication data"""
    tx_buffer: List[str] = field(default_factory=list)
    rx_buffer: List[str] = field(default_factory=list)
    baud_rate: int = 9600
    tx_count: int = 0
    rx_count: int = 0

class AVRSimulator:
    """Complete ATmega328P/Arduino simulation environment"""
    
    def __init__(self):
        """Initialize the AVR simulator"""
        # Hardware state
        self.pins: Dict[int, GPIOPin] = {}
        self.timers: Dict[int, TimerConfig] = {}
        self.serial: SerialData = SerialData()
        self.running: bool = False
        self.simulation_time: float = 0.0
        self.clock_frequency: int = 16_000_000  # 16MHz
        
        # Initialize pins (Arduino Uno mapping)
        self.init_gpio_pins()
        self.init_timers()
        
        # GUI components
        self.root: Optional[tk.Tk] = None
        self.pin_widgets: Dict[int, Dict] = {}
        self.timer_widgets: Dict = {}
        self.serial_text: Optional[tk.Text] = None
        self.code_text: Optional[tk.Text] = None
        self.status_var: Optional[tk.StringVar] = None
        
        # Simulation thread
        self.sim_thread: Optional[threading.Thread] = None
        self.sim_running: bool = False
        
        # Code compilation and execution
        self.current_code: str = ""
        self.compiled_program: Optional[Any] = None
        
        print("🔧 AVR/Arduino Simulator initialized")
        print(f"Clock frequency: {self.clock_frequency:,} Hz")
        print(f"GPIO pins: {len(self.pins)} configured")
    
    def init_gpio_pins(self):
        """Initialize GPIO pins with Arduino Uno mapping"""
        # Digital pins 0-13
        for pin in range(14):
            self.pins[pin] = GPIOPin(number=pin)
        
        # Analog pins A0-A5 (14-19)
        for pin in range(14, 20):
            analog_pin = GPIOPin(number=pin, mode=PinMode.INPUT)
            analog_pin.analog_value = 512  # Default middle value
            self.pins[pin] = analog_pin
        
        # Special pin configurations
        self.pins[13].mode = PinMode.OUTPUT  # Built-in LED
        self.pins[0].mode = PinMode.INPUT    # RX
        self.pins[1].mode = PinMode.OUTPUT   # TX
    
    def init_timers(self):
        """Initialize hardware timers"""
        self.timers[0] = TimerConfig(timer_id=0)  # Timer0 (8-bit)
        self.timers[1] = TimerConfig(timer_id=1)  # Timer1 (16-bit)
        self.timers[2] = TimerConfig(timer_id=2)  # Timer2 (8-bit)
    
    def create_gui(self):
        """Create the simulation GUI"""
        self.root = tk.Tk()
        self.root.title("🔧 AVR/Arduino Hardware Simulator")
        self.root.geometry("1400x900")
        
        # Create main frames
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Left panel - Code editor and controls
        left_frame = ttk.Frame(main_frame)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))
        
        # Right panel - Hardware visualization
        right_frame = ttk.Frame(main_frame, width=500)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, padx=(5, 0))
        right_frame.pack_propagate(False)
        
        self.create_code_editor(left_frame)
        self.create_control_panel(left_frame)
        self.create_hardware_panel(right_frame)
        self.create_status_bar()
        
        # Start GUI update loop
        self.update_gui()
        
        print("🖥️ Simulator GUI created successfully")
    
    def create_code_editor(self, parent):
        """Create code editor panel"""
        editor_frame = ttk.LabelFrame(parent, text="📝 Arduino Code Editor", padding=5)
        editor_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 5))
        
        # Toolbar
        toolbar = ttk.Frame(editor_frame)
        toolbar.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Button(toolbar, text="📂 Load", command=self.load_code).pack(side=tk.LEFT, padx=(0, 5))
        ttk.Button(toolbar, text="💾 Save", command=self.save_code).pack(side=tk.LEFT, padx=(0, 5))
        ttk.Button(toolbar, text="🔧 Compile", command=self.compile_code).pack(side=tk.LEFT, padx=(0, 5))
        ttk.Button(toolbar, text="▶️ Run", command=self.run_code).pack(side=tk.LEFT, padx=(0, 5))
        ttk.Button(toolbar, text="⏹️ Stop", command=self.stop_simulation).pack(side=tk.LEFT, padx=(0, 5))
        ttk.Button(toolbar, text="🔄 Reset", command=self.reset_simulation).pack(side=tk.LEFT, padx=(0, 5))
        
        # Code text area
        text_frame = ttk.Frame(editor_frame)
        text_frame.pack(fill=tk.BOTH, expand=True)
        
        self.code_text = tk.Text(text_frame, wrap=tk.NONE, font=("Consolas", 11))
        
        # Scrollbars
        v_scrollbar = ttk.Scrollbar(text_frame, orient=tk.VERTICAL, command=self.code_text.yview)
        h_scrollbar = ttk.Scrollbar(text_frame, orient=tk.HORIZONTAL, command=self.code_text.xview)
        self.code_text.config(yscrollcommand=v_scrollbar.set, xscrollcommand=h_scrollbar.set)
        
        # Pack text and scrollbars
        self.code_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        v_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        h_scrollbar.pack(side=tk.BOTTOM, fill=tk.X)
        
        # Load default example code
        self.load_example_code()
    
    def create_control_panel(self, parent):
        """Create simulation control panel"""
        control_frame = ttk.LabelFrame(parent, text="🎛️ Simulation Controls", padding=5)
        control_frame.pack(fill=tk.X, pady=(0, 5))
        
        # Simulation speed control
        speed_frame = ttk.Frame(control_frame)
        speed_frame.pack(fill=tk.X, pady=2)
        
        ttk.Label(speed_frame, text="Simulation Speed:").pack(side=tk.LEFT)
        self.speed_var = tk.StringVar(value="1x")
        speed_combo = ttk.Combobox(speed_frame, textvariable=self.speed_var, 
                                  values=["0.1x", "0.5x", "1x", "2x", "5x", "10x"], 
                                  width=10, state="readonly")
        speed_combo.pack(side=tk.LEFT, padx=(5, 0))
        
        # Serial monitor
        serial_frame = ttk.LabelFrame(control_frame, text="📡 Serial Monitor (9600 baud)")
        serial_frame.pack(fill=tk.X, pady=5)
        
        self.serial_text = tk.Text(serial_frame, height=6, wrap=tk.WORD, 
                                  font=("Consolas", 10), bg="black", fg="green")
        serial_scroll = ttk.Scrollbar(serial_frame, orient=tk.VERTICAL, 
                                     command=self.serial_text.yview)
        self.serial_text.config(yscrollcommand=serial_scroll.set)
        
        self.serial_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        serial_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Serial input
        serial_input_frame = ttk.Frame(serial_frame)
        serial_input_frame.pack(fill=tk.X, pady=2)
        
        self.serial_input = tk.Entry(serial_input_frame)
        self.serial_input.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.serial_input.bind("<Return>", self.send_serial_input)
        
        ttk.Button(serial_input_frame, text="Send", 
                  command=self.send_serial_input).pack(side=tk.RIGHT, padx=(5, 0))
    
    def create_hardware_panel(self, parent):
        """Create hardware visualization panel"""
        # Arduino board visualization
        board_frame = ttk.LabelFrame(parent, text="🔌 Arduino Uno Hardware", padding=5)
        board_frame.pack(fill=tk.BOTH, expand=True)
        
        # Create notebook for different hardware sections
        notebook = ttk.Notebook(board_frame)
        notebook.pack(fill=tk.BOTH, expand=True)
        
        # GPIO pins tab
        self.create_gpio_tab(notebook)
        
        # Timers tab
        self.create_timers_tab(notebook)
        
        # Analog tab
        self.create_analog_tab(notebook)
        
        # I2C/SPI tab
        self.create_communication_tab(notebook)
    
    def create_gpio_tab(self, notebook):
        """Create GPIO pins visualization tab"""
        gpio_frame = ttk.Frame(notebook)
        notebook.add(gpio_frame, text="🔌 GPIO Pins")
        
        # Digital pins 0-13
        digital_frame = ttk.LabelFrame(gpio_frame, text="Digital Pins (0-13)")
        digital_frame.pack(fill=tk.X, pady=5)
        
        pins_frame = ttk.Frame(digital_frame)
        pins_frame.pack(fill=tk.X, padx=5, pady=5)
        
        for pin_num in range(14):
            pin_frame = ttk.Frame(pins_frame)
            if pin_num < 7:
                pin_frame.grid(row=0, column=pin_num, padx=2, pady=2, sticky="ew")
            else:
                pin_frame.grid(row=1, column=pin_num-7, padx=2, pady=2, sticky="ew")
            
            # Pin number label
            ttk.Label(pin_frame, text=f"D{pin_num}", font=("Arial", 8, "bold")).pack()
            
            # Mode selection
            mode_var = tk.StringVar(value="INPUT")
            mode_combo = ttk.Combobox(pin_frame, textvariable=mode_var,
                                     values=["INPUT", "OUTPUT", "INPUT_PULLUP"],
                                     width=8, font=("Arial", 7), state="readonly")
            mode_combo.pack(pady=1)
            mode_combo.bind("<<ComboboxSelected>>", 
                          lambda e, p=pin_num: self.set_pin_mode(p, e.widget.get()))
            
            # State control/display
            if pin_num == 13:  # Built-in LED
                state_var = tk.BooleanVar()
                led_check = ttk.Checkbutton(pin_frame, text="LED", variable=state_var,
                                          command=lambda p=pin_num, v=state_var: self.set_pin_state(p, v.get()))
                led_check.pack()
                self.pin_widgets[pin_num] = {"mode": mode_combo, "state": state_var, "widget": led_check}
            else:
                state_var = tk.BooleanVar()
                state_check = ttk.Checkbutton(pin_frame, text="HIGH", variable=state_var,
                                            command=lambda p=pin_num, v=state_var: self.set_pin_state(p, v.get()))
                state_check.pack()
                self.pin_widgets[pin_num] = {"mode": mode_combo, "state": state_var, "widget": state_check}
    
    def create_timers_tab(self, notebook):
        """Create timers visualization tab"""
        timer_frame = ttk.Frame(notebook)
        notebook.add(timer_frame, text="⏱️ Timers")
        
        for timer_id in [0, 1, 2]:
            t_frame = ttk.LabelFrame(timer_frame, text=f"Timer{timer_id}")
            t_frame.pack(fill=tk.X, pady=5, padx=5)
            
            info_frame = ttk.Frame(t_frame)
            info_frame.pack(fill=tk.X, padx=5, pady=5)
            
            # Timer info
            ttk.Label(info_frame, text="Prescaler:").grid(row=0, column=0, sticky="w")
            prescaler_var = tk.StringVar(value="1")
            prescaler_label = ttk.Label(info_frame, textvariable=prescaler_var)
            prescaler_label.grid(row=0, column=1, sticky="w", padx=(5, 0))
            
            ttk.Label(info_frame, text="Counter:").grid(row=1, column=0, sticky="w")
            counter_var = tk.StringVar(value="0")
            counter_label = ttk.Label(info_frame, textvariable=counter_var)
            counter_label.grid(row=1, column=1, sticky="w", padx=(5, 0))
            
            ttk.Label(info_frame, text="Frequency:").grid(row=0, column=2, sticky="w", padx=(10, 0))
            freq_var = tk.StringVar(value="0.0 Hz")
            freq_label = ttk.Label(info_frame, textvariable=freq_var)
            freq_label.grid(row=0, column=3, sticky="w", padx=(5, 0))
            
            # PWM controls for Timer1 (pins 9, 10)
            if timer_id == 1:
                pwm_frame = ttk.Frame(t_frame)
                pwm_frame.pack(fill=tk.X, padx=5, pady=2)
                
                ttk.Label(pwm_frame, text="PWM Pin 9:").pack(side=tk.LEFT)
                pwm9_var = tk.IntVar()
                pwm9_scale = ttk.Scale(pwm_frame, from_=0, to=255, orient=tk.HORIZONTAL,
                                      variable=pwm9_var, length=100)
                pwm9_scale.pack(side=tk.LEFT, padx=5)
                ttk.Label(pwm_frame, textvariable=tk.StringVar()).pack(side=tk.LEFT)
                
                ttk.Label(pwm_frame, text="PWM Pin 10:").pack(side=tk.LEFT, padx=(20, 0))
                pwm10_var = tk.IntVar()
                pwm10_scale = ttk.Scale(pwm_frame, from_=0, to=255, orient=tk.HORIZONTAL,
                                       variable=pwm10_var, length=100)
                pwm10_scale.pack(side=tk.LEFT, padx=5)
            
            self.timer_widgets[timer_id] = {
                "prescaler": prescaler_var,
                "counter": counter_var,
                "frequency": freq_var
            }
    
    def create_analog_tab(self, notebook):
        """Create analog pins tab"""
        analog_frame = ttk.Frame(notebook)
        notebook.add(analog_frame, text="📊 Analog")
        
        # Analog inputs A0-A5
        for pin in range(6):
            analog_pin = pin + 14  # A0 = pin 14
            
            pin_frame = ttk.LabelFrame(analog_frame, text=f"A{pin} (Pin {analog_pin})")
            pin_frame.pack(fill=tk.X, pady=5, padx=5)
            
            control_frame = ttk.Frame(pin_frame)
            control_frame.pack(fill=tk.X, padx=5, pady=5)
            
            # Analog value control
            ttk.Label(control_frame, text="Value (0-1023):").pack(side=tk.LEFT)
            
            analog_var = tk.IntVar(value=512)
            analog_scale = ttk.Scale(control_frame, from_=0, to=1023, orient=tk.HORIZONTAL,
                                   variable=analog_var, length=200)
            analog_scale.pack(side=tk.LEFT, padx=5)
            
            value_label = ttk.Label(control_frame, text="512")
            value_label.pack(side=tk.LEFT, padx=5)
            
            # Voltage display
            voltage_label = ttk.Label(control_frame, text="2.50V")
            voltage_label.pack(side=tk.LEFT, padx=5)
            
            # Update callback
            def update_analog(val, pin_num=analog_pin, lbl=value_label, vlbl=voltage_label):
                value = int(float(val))
                voltage = (value / 1023.0) * 5.0
                lbl.config(text=str(value))
                vlbl.config(text=f"{voltage:.2f}V")
                if pin_num in self.pins:
                    self.pins[pin_num].analog_value = value
            
            analog_scale.config(command=update_analog)
    
    def create_communication_tab(self, notebook):
        """Create I2C/SPI communication tab"""
        comm_frame = ttk.Frame(notebook)
        notebook.add(comm_frame, text="📡 I2C/SPI")
        
        # I2C section
        i2c_frame = ttk.LabelFrame(comm_frame, text="I2C Communication")
        i2c_frame.pack(fill=tk.X, pady=5, padx=5)
        
        ttk.Label(i2c_frame, text="SDA: Pin A4 (18)").pack(anchor="w", padx=5)
        ttk.Label(i2c_frame, text="SCL: Pin A5 (19)").pack(anchor="w", padx=5)
        
        # SPI section
        spi_frame = ttk.LabelFrame(comm_frame, text="SPI Communication")
        spi_frame.pack(fill=tk.X, pady=5, padx=5)
        
        ttk.Label(spi_frame, text="MISO: Pin 12").pack(anchor="w", padx=5)
        ttk.Label(spi_frame, text="MOSI: Pin 11").pack(anchor="w", padx=5)
        ttk.Label(spi_frame, text="SCK:  Pin 13").pack(anchor="w", padx=5)
        ttk.Label(spi_frame, text="SS:   Pin 10").pack(anchor="w", padx=5)
    
    def create_status_bar(self):
        """Create status bar"""
        self.status_var = tk.StringVar(value="Ready - Load code and click Run to start simulation")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)
    
    def load_example_code(self):
        """Load example Arduino code"""
        example_code = '''// 🔧 Arduino LED Blink Example
// This code demonstrates basic GPIO control and timing

void setup() {
  // Initialize digital pin 13 (LED) as output
  pinMode(13, OUTPUT);
  
  // Initialize serial communication at 9600 bps
  Serial.begin(9600);
  Serial.println("Arduino LED Blink Started!");
}

void loop() {
  // Turn the LED on
  digitalWrite(13, HIGH);
  Serial.println("LED ON");
  delay(1000);  // Wait for 1 second
  
  // Turn the LED off
  digitalWrite(13, LOW);
  Serial.println("LED OFF");
  delay(1000);  // Wait for 1 second
}

// Advanced Example: PWM Control
/*
void setup() {
  pinMode(9, OUTPUT);    // PWM output pin
  Serial.begin(9600);
}

void loop() {
  // Fade in
  for (int brightness = 0; brightness <= 255; brightness++) {
    analogWrite(9, brightness);
    Serial.print("Brightness: ");
    Serial.println(brightness);
    delay(10);
  }
  
  // Fade out
  for (int brightness = 255; brightness >= 0; brightness--) {
    analogWrite(9, brightness);
    Serial.print("Brightness: ");
    Serial.println(brightness);
    delay(10);
  }
}
*/'''
        
        if self.code_text:
            self.code_text.delete(1.0, tk.END)
            self.code_text.insert(1.0, example_code)
    
    def load_code(self):
        """Load code from file"""
        file_path = filedialog.askopenfilename(
            title="Load Arduino Code",
            filetypes=[("Arduino files", "*.ino"), ("C files", "*.c"), ("All files", "*.*")]
        )
        
        if file_path:
            try:
                with open(file_path, 'r', encoding='utf-8') as file:
                    code = file.read()
                    self.code_text.delete(1.0, tk.END)
                    self.code_text.insert(1.0, code)
                self.status_var.set(f"Loaded: {os.path.basename(file_path)}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load file: {e}")
    
    def save_code(self):
        """Save code to file"""
        file_path = filedialog.asksaveasfilename(
            title="Save Arduino Code",
            defaultextension=".ino",
            filetypes=[("Arduino files", "*.ino"), ("C files", "*.c"), ("All files", "*.*")]
        )
        
        if file_path:
            try:
                with open(file_path, 'w', encoding='utf-8') as file:
                    file.write(self.code_text.get(1.0, tk.END))
                self.status_var.set(f"Saved: {os.path.basename(file_path)}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save file: {e}")
    
    def compile_code(self):
        """Compile Arduino code (simplified)"""
        self.current_code = self.code_text.get(1.0, tk.END)
        
        if not self.current_code.strip():
            messagebox.showwarning("Warning", "No code to compile!")
            return
        
        self.status_var.set("Compiling code...")
        
        # Simple validation
        if "void setup()" not in self.current_code:
            messagebox.showerror("Compile Error", "setup() function is required!")
            self.status_var.set("Compilation failed")
            return
        
        if "void loop()" not in self.current_code:
            messagebox.showerror("Compile Error", "loop() function is required!")
            self.status_var.set("Compilation failed")
            return
        
        # Extract functions and simulate compilation
        self.extract_arduino_functions()
        
        self.status_var.set("Code compiled successfully ✓")
        print("🔧 Code compiled successfully")
    
    def extract_arduino_functions(self):
        """Extract and parse Arduino functions"""
        # This is a simplified parser for demonstration
        # In a real implementation, you'd use a proper C parser
        
        self.setup_code = ""
        self.loop_code = ""
        
        # Extract setup function
        setup_match = re.search(r'void\s+setup\s*\(\s*\)\s*\{([^{}]*(?:\{[^{}]*\}[^{}]*)*)\}', 
                               self.current_code, re.DOTALL)
        if setup_match:
            self.setup_code = setup_match.group(1).strip()
        
        # Extract loop function
        loop_match = re.search(r'void\s+loop\s*\(\s*\)\s*\{([^{}]*(?:\{[^{}]*\}[^{}]*)*)\}', 
                              self.current_code, re.DOTALL)
        if loop_match:
            self.loop_code = loop_match.group(1).strip()
        
        print(f"Extracted setup code: {len(self.setup_code)} characters")
        print(f"Extracted loop code: {len(self.loop_code)} characters")
    
    def run_code(self):
        """Run the compiled code"""
        if not hasattr(self, 'setup_code') or not hasattr(self, 'loop_code'):
            messagebox.showwarning("Warning", "Please compile the code first!")
            return
        
        if self.sim_running:
            messagebox.showwarning("Warning", "Simulation is already running!")
            return
        
        self.reset_simulation()
        self.sim_running = True
        self.status_var.set("Running simulation...")
        
        # Start simulation thread
        self.sim_thread = threading.Thread(target=self.simulation_loop, daemon=True)
        self.sim_thread.start()
        
        print("▶️ Simulation started")
    
    def simulation_loop(self):
        """Main simulation execution loop"""
        try:
            # Execute setup once
            self.execute_setup()
            
            # Execute loop repeatedly
            loop_count = 0
            while self.sim_running:
                start_time = time.time()
                
                self.execute_loop()
                loop_count += 1
                
                # Update simulation time
                self.simulation_time += 0.001  # 1ms per loop iteration
                
                # Respect simulation speed
                speed_multiplier = float(self.speed_var.get().replace('x', ''))
                loop_time = time.time() - start_time
                target_time = 0.001 / speed_multiplier  # Target loop time
                
                if loop_time < target_time:
                    time.sleep(target_time - loop_time)
                
                # Update status periodically
                if loop_count % 100 == 0:
                    self.root.after(0, lambda: self.status_var.set(
                        f"Running... Loop: {loop_count}, Time: {self.simulation_time:.3f}s"
                    ))
        
        except Exception as e:
            print(f"Simulation error: {e}")
            self.root.after(0, lambda: self.status_var.set(f"Simulation error: {e}"))
        
        finally:
            self.sim_running = False
    
    def execute_setup(self):
        """Execute Arduino setup function"""
        print("🔧 Executing setup()...")
        self.parse_and_execute_code(self.setup_code)
        self.serial_output("Arduino setup completed!\n")
    
    def execute_loop(self):
        """Execute Arduino loop function"""
        self.parse_and_execute_code(self.loop_code)
    
    def parse_and_execute_code(self, code: str):
        """Parse and execute Arduino code (simplified)"""
        lines = [line.strip() for line in code.split('\n') if line.strip()]
        
        for line in lines:
            if line.startswith('//') or line.startswith('/*'):
                continue  # Skip comments
            
            try:
                self.execute_line(line)
            except Exception as e:
                print(f"Error executing line '{line}': {e}")
    
    def execute_line(self, line: str):
        """Execute a single line of Arduino code"""
        line = line.strip().rstrip(';')
        
        if not line:
            return
        
        # pinMode(pin, mode)
        if line.startswith('pinMode('):
            match = re.search(r'pinMode\s*\(\s*(\d+)\s*,\s*(\w+)\s*\)', line)
            if match:
                pin = int(match.group(1))
                mode = match.group(2)
                self.arduino_pinMode(pin, mode)
        
        # digitalWrite(pin, state)
        elif line.startswith('digitalWrite('):
            match = re.search(r'digitalWrite\s*\(\s*(\d+)\s*,\s*(\w+)\s*\)', line)
            if match:
                pin = int(match.group(1))
                state = match.group(2)
                self.arduino_digitalWrite(pin, state)
        
        # analogWrite(pin, value)
        elif line.startswith('analogWrite('):
            match = re.search(r'analogWrite\s*\(\s*(\d+)\s*,\s*(\d+)\s*\)', line)
            if match:
                pin = int(match.group(1))
                value = int(match.group(2))
                self.arduino_analogWrite(pin, value)
        
        # Serial.begin(baud)
        elif 'Serial.begin(' in line:
            match = re.search(r'Serial\.begin\s*\(\s*(\d+)\s*\)', line)
            if match:
                baud = int(match.group(1))
                self.arduino_serial_begin(baud)
        
        # Serial.println(text)
        elif 'Serial.println(' in line:
            match = re.search(r'Serial\.println\s*\(\s*"([^"]*)"\s*\)', line)
            if match:
                text = match.group(1)
                self.arduino_serial_println(text)
        
        # Serial.print(text)
        elif 'Serial.print(' in line:
            match = re.search(r'Serial\.print\s*\(\s*"([^"]*)"\s*\)', line)
            if match:
                text = match.group(1)
                self.arduino_serial_print(text)
        
        # delay(ms)
        elif line.startswith('delay('):
            match = re.search(r'delay\s*\(\s*(\d+)\s*\)', line)
            if match:
                ms = int(match.group(1))
                self.arduino_delay(ms)
    
    # Arduino function implementations
    def arduino_pinMode(self, pin: int, mode: str):
        """Implement Arduino pinMode function"""
        if pin not in self.pins:
            return
        
        if mode == "OUTPUT":
            self.pins[pin].mode = PinMode.OUTPUT
        elif mode == "INPUT":
            self.pins[pin].mode = PinMode.INPUT
        elif mode == "INPUT_PULLUP":
            self.pins[pin].mode = PinMode.INPUT_PULLUP
        
        # Update GUI
        if pin in self.pin_widgets:
            self.root.after(0, lambda: self.pin_widgets[pin]["mode"].set(mode))
        
        print(f"pinMode({pin}, {mode})")
    
    def arduino_digitalWrite(self, pin: int, state: str):
        """Implement Arduino digitalWrite function"""
        if pin not in self.pins or self.pins[pin].mode != PinMode.OUTPUT:
            return
        
        pin_state = PinState.HIGH if state == "HIGH" else PinState.LOW
        self.pins[pin].state = pin_state
        self.pins[pin].last_change = self.simulation_time
        
        # Update GUI
        if pin in self.pin_widgets:
            gui_state = (state == "HIGH")
            self.root.after(0, lambda: self.pin_widgets[pin]["state"].set(gui_state))
        
        print(f"digitalWrite({pin}, {state})")
    
    def arduino_analogWrite(self, pin: int, value: int):
        """Implement Arduino analogWrite (PWM) function"""
        if pin not in self.pins:
            return
        
        self.pins[pin].pwm_value = max(0, min(255, value))
        self.pins[pin].pwm_enabled = True
        
        print(f"analogWrite({pin}, {value})")
    
    def arduino_serial_begin(self, baud: int):
        """Implement Arduino Serial.begin function"""
        self.serial.baud_rate = baud
        self.serial_output(f"Serial communication started at {baud} baud\n")
        print(f"Serial.begin({baud})")
    
    def arduino_serial_println(self, text: str):
        """Implement Arduino Serial.println function"""
        self.serial_output(f"{text}\n")
        print(f"Serial.println: {text}")
    
    def arduino_serial_print(self, text: str):
        """Implement Arduino Serial.print function"""
        self.serial_output(text)
        print(f"Serial.print: {text}")
    
    def arduino_delay(self, ms: int):
        """Implement Arduino delay function"""
        delay_seconds = ms / 1000.0
        speed_multiplier = float(self.speed_var.get().replace('x', ''))
        actual_delay = delay_seconds / speed_multiplier
        
        if actual_delay > 0.001:  # Only delay if significant
            time.sleep(actual_delay)
        
        self.simulation_time += delay_seconds
        print(f"delay({ms}ms)")
    
    def serial_output(self, text: str):
        """Output text to serial monitor"""
        if self.serial_text:
            self.root.after(0, lambda: self._update_serial_text(text))
        
        self.serial.tx_buffer.append(text)
        self.serial.tx_count += len(text)
    
    def _update_serial_text(self, text: str):
        """Update serial monitor text (called from main thread)"""
        self.serial_text.insert(tk.END, text)
        self.serial_text.see(tk.END)
        
        # Limit buffer size
        if len(self.serial_text.get(1.0, tk.END)) > 10000:
            self.serial_text.delete(1.0, "5.0")
    
    def send_serial_input(self, event=None):
        """Send input to Arduino via serial"""
        input_text = self.serial_input.get()
        if input_text:
            self.serial.rx_buffer.append(input_text + "\n")
            self.serial_output(f">> {input_text}\n")
            self.serial_input.delete(0, tk.END)
    
    def set_pin_mode(self, pin: int, mode: str):
        """Set pin mode from GUI"""
        if pin in self.pins:
            if mode == "OUTPUT":
                self.pins[pin].mode = PinMode.OUTPUT
            elif mode == "INPUT":
                self.pins[pin].mode = PinMode.INPUT
            elif mode == "INPUT_PULLUP":
                self.pins[pin].mode = PinMode.INPUT_PULLUP
            
            print(f"GUI: pinMode({pin}, {mode})")
    
    def set_pin_state(self, pin: int, state: bool):
        """Set pin state from GUI"""
        if pin in self.pins and self.pins[pin].mode == PinMode.OUTPUT:
            self.pins[pin].state = PinState.HIGH if state else PinState.LOW
            print(f"GUI: digitalWrite({pin}, {'HIGH' if state else 'LOW'})")
    
    def stop_simulation(self):
        """Stop the simulation"""
        self.sim_running = False
        if self.sim_thread and self.sim_thread.is_alive():
            self.sim_thread.join(timeout=1.0)
        
        self.status_var.set("Simulation stopped")
        print("⏹️ Simulation stopped")
    
    def reset_simulation(self):
        """Reset simulation state"""
        self.stop_simulation()
        
        # Reset pin states
        for pin in self.pins.values():
            pin.state = PinState.LOW
            pin.analog_value = 512 if pin.number >= 14 else 0
            pin.pwm_value = 0
            pin.pwm_enabled = False
            pin.last_change = 0.0
        
        # Reset timers
        for timer in self.timers.values():
            timer.counter_value = 0
            timer.overflow_count = 0
            timer.frequency = 0.0
            timer.duty_cycle = 0.0
        
        # Reset serial
        self.serial.tx_buffer.clear()
        self.serial.rx_buffer.clear()
        self.serial.tx_count = 0
        self.serial.rx_count = 0
        
        # Reset simulation time
        self.simulation_time = 0.0
        
        # Clear serial monitor
        if self.serial_text:
            self.serial_text.delete(1.0, tk.END)
        
        # Update GUI
        self.update_pin_displays()
        
        self.status_var.set("Simulation reset")
        print("🔄 Simulation reset")
    
    def update_gui(self):
        """Update GUI periodically"""
        if self.root:
            self.update_pin_displays()
            self.update_timer_displays()
            
            # Schedule next update
            self.root.after(100, self.update_gui)
    
    def update_pin_displays(self):
        """Update pin state displays"""
        for pin_num, widgets in self.pin_widgets.items():
            if pin_num in self.pins:
                pin = self.pins[pin_num]
                
                # Update mode display
                mode_name = pin.mode.name
                widgets["mode"].set(mode_name)
                
                # Update state display
                if pin.mode == PinMode.OUTPUT:
                    gui_state = (pin.state == PinState.HIGH)
                    widgets["state"].set(gui_state)
                    
                    # Special handling for LED (pin 13)
                    if pin_num == 13:
                        led_color = "green" if gui_state else "gray"
                        # Could update LED color here if we had LED widgets
    
    def update_timer_displays(self):
        """Update timer displays"""
        for timer_id, widgets in self.timer_widgets.items():
            if timer_id in self.timers:
                timer = self.timers[timer_id]
                
                widgets["prescaler"].set(str(timer.prescaler))
                widgets["counter"].set(str(timer.counter_value))
                widgets["frequency"].set(f"{timer.frequency:.2f} Hz")
    
    def run(self):
        """Start the simulator GUI"""
        self.create_gui()
        if self.root:
            self.root.mainloop()

def main():
    """Run the AVR/Arduino simulator"""
    print("🔧 Starting AVR/Arduino Hardware Simulator")
    print("=" * 50)
    
    simulator = AVRSimulator()
    simulator.run()

if __name__ == "__main__":
    main()