import threading
import can
import tkinter as tk
import cantools

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

        self.update_thread = threading.Thread(target=self.receive_can_messages)
        self.update_thread.daemon = True
        self.update_thread.start()

    def update_voltages(self, voltages):
        for i, voltage in enumerate(voltages):
            color = self.get_voltage_color(voltage)
            self.voltage_labels[i].config(text=f"Cell {i+1} Voltage: {voltage:.2f} V", fg=color)

    def update_temperatures(self, temperatures):
        for i, temp in enumerate(temperatures):
            color = 'black'
            if temp > 58:
                color = 'red'
            elif temp > 45:
                color = 'yellow'
            self.temp_labels[i].config(text=f"Cell {i+1} Temp: {temp:.2f} C", bg=color)

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

if __name__ == "__main__":
    bus = can.interface.Bus(interface='kvaser', channel='0', bitrate=500000)
    dbc_file = 'dbc/ams.dbc'
    db = cantools.database.load_file(dbc_file)
    root = tk.Tk()
    app = BatteryMonitorApp(root, bus, db)
    root.mainloop() 