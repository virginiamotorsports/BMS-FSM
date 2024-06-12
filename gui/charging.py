import threading
import can
import tkinter as tk
import cantools

temps = [[None for _ in range(8)] for _ in range(6)]
voltages = [[None for _ in range(16)] for _ in range(6)]

def receive_can_messages(bus):
    while True:
        message = bus.recv()
        print(f"Received message: {message}")

class BatteryMonitorApp:
    def __init__(self, root, bus, db):
        self.root = root
        self.bus = bus
        self.db = db
        self.root.title("Battery Monitor")
        
        self.voltage_labels = []
        self.temp_labels = []

        for group in range(6):  # 96 voltages / 16 = 6 groups
            for i in range(16):
                voltage_label = tk.Label(root, text=f"S{group+1} C{i+1} : -- V", width=10)
                voltage_label.grid(row=i, column=group * 2)
                self.voltage_labels.append(voltage_label)
        
        for group in range(6):  # 48 temperatures / 8 = 6 groups
            for i in range(8):
                temp_label = tk.Label(root, text=f"S{group+1} T{i+1} S: -- C", width=20)
                temp_label.grid(row=i, column=group * 2 + 1)
                self.temp_labels.append(temp_label)
                
        # Add labels for min, max temp, min, max voltage, and sum of voltages
        self.min_temp_label = tk.Label(root, text="Min Temp: -- C", width=30)
        self.min_temp_label.grid(row=48, column=0, columnspan=2, sticky='ew')
        self.max_temp_label = tk.Label(root, text="Max Temp: -- C", width=30)
        self.max_temp_label.grid(row=48, column=2, columnspan=2, sticky='ew')
        self.min_voltage_label = tk.Label(root, text="Min Voltage: -- V", width=30)
        self.min_voltage_label.grid(row=49, column=0, columnspan=2, sticky='ew')
        self.max_voltage_label = tk.Label(root, text="Max Voltage: -- V", width=30)
        self.max_voltage_label.grid(row=49, column=2, columnspan=2, sticky='ew')
        self.sum_voltage_label = tk.Label(root, text="Sum Voltage: -- V", width=30)
        self.sum_voltage_label.grid(row=50, column=0, columnspan=4, sticky='ew')

        self.update_thread = threading.Thread(target=self.receive_can_messages)
        self.update_thread.daemon = True
        self.update_thread.start()

    def update_voltages(self):
        flat_voltages = [v for sublist in voltages for v in sublist if v is not None]
        if flat_voltages:
            min_voltage = min(flat_voltages)
            max_voltage = max(flat_voltages)
            sum_voltage = sum(flat_voltages)
            self.min_voltage_label.config(text=f"Min Voltage: {min_voltage:.2f} V")
            self.max_voltage_label.config(text=f"Max Voltage: {max_voltage:.2f} V")
            self.sum_voltage_label.config(text=f"Sum Voltage: {sum_voltage:.2f} V")
            
        for group in range(6):
            for i in range(16):
                voltage = voltages[group][i]
                if voltage is not None:
                    color = self.get_voltage_color(voltage)
                    self.voltage_labels[group * 16 + i].config(text=f"C {group*16+i+1} V: {voltage:.2f} V", fg=color)

    def update_temperatures(self):
        flat_temps = [t for sublist in temps for t in sublist if t is not None]
        if flat_temps:
            min_temp = min(flat_temps)
            max_temp = max(flat_temps)
            self.min_temp_label.config(text=f"Min Temp: {min_temp:.2f} C")
            self.max_temp_label.config(text=f"Max Temp: {max_temp:.2f} C")
            
        for group in range(6):
            for i in range(8):
                temp = temps[group][i]
                if temp is not None:
                    color = 'black'
                    if temp > 58:
                        color = 'red'
                    elif temp > 45:
                        color = 'yellow'
                    self.temp_labels[group * 8 + i].config(text=f"C {group*8+i+1} T: {temp:.2f} C", fg=color)

    def receive_can_messages(self):
        while True:
            message = self.bus.recv()
            if message is not None:
                decoded_message = self.db.decode_message(message.arbitration_id, message.data)
                self.process_message(decoded_message)

    def get_voltage_color(self, voltage):
        # Map the voltage to a color between green (4.2V) and red (3.0V)
        min_voltage = 3.0
        max_voltage = 4.2
        if voltage <= min_voltage:
            return "#FF0000"  # Red
        elif voltage >= max_voltage:
            return "#00FF00"  # Green
        else:
            ratio = (voltage - min_voltage) / (max_voltage - min_voltage)
            red = int(255 * (1 - ratio))
            green = int(255 * ratio)
            return f'#{red:02x}{green:02x}00'

    def process_message(self, decoded_message):
        global temps, voltages
        group_updated = None

        for key, value in decoded_message.items():
            if 'Temp' in key:
                index = int(key[4:]) - 1
                group = index // 8
                sub_index = index % 8
                temps[group][sub_index] = value
                group_updated = group
            elif 'Cell' in key:
                index = int(key[4:]) - 1
                group = index // 16
                sub_index = index % 16
                voltages[group][sub_index] = value
                group_updated = group

        # Update the GUI after the last voltage message in each group
        if group_updated is not None:
            if all(voltages[group_updated]) and all(temps[group_updated]):
                self.update_voltages()
                self.update_temperatures()

if __name__ == "__main__":
    bus = can.interface.Bus(interface='kvaser', channel='0', bitrate=500000)
    dbc_file = 'dbc/ams.dbc'
    db = cantools.database.load_file(dbc_file)
    root = tk.Tk()
    app = BatteryMonitorApp(root, bus, db)
    root.mainloop() 