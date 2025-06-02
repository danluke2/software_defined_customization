import sys
import cfg
import subprocess
import threading
import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox

from CIB_helper import *
import logging
import ipaddress

logger = logging.getLogger(__name__)

CPCON_SCRIPTS = {
    1: None,
    2: "CPCON_scripts/cpcon_level_2.sh",
    3: "CPCON_scripts/cpcon_level_3.sh",
    4: None,
    5: None,
}


class NCO_UI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("NCO Control Panel")
        self.geometry("800x600")

        try:
            self.db_connection = db_connect(cfg.nco_dir + "cib.db")
        except Exception as e:
            messagebox.showerror(
                "Database Error", f"Unable to connect to database:\n{e}"
            )
            sys.exit()

        self.create_widgets()

    def create_widgets(self):
        # CPCON Level Dropdown
        tk.Label(self, text="CPCON Level:").pack(pady=(10, 0))
        self.cpcon_level = ttk.Combobox(self, values=[1, 2, 3, 4, 5], state="readonly")
        self.cpcon_level.pack()

        # Threat Intelligence Dropdown
        tk.Label(self, text="Threat Type:").pack(pady=(10, 0))
        self.threat_type = ttk.Combobox(
            self,
            values=["Denial of Service", "Web attacks", "Phishing", "Other"],
            state="readonly",
        )
        self.threat_type.pack()

        # Submit Button
        tk.Button(self, text="Set CPCON", command=self.set_cpcon).pack(pady=10)

        # Buttons
        button_frame = tk.Frame(self)
        button_frame.pack(pady=10)

        tk.Button(button_frame, text="View Hosts", command=self.view_hosts).pack(
            side=tk.LEFT, padx=5
        )
        tk.Button(button_frame, text="View Policies", command=self.view_policies).pack(
            side=tk.LEFT, padx=5
        )
        tk.Button(button_frame, text="View Alerts", command=self.view_alerts).pack(
            side=tk.LEFT, padx=5
        )

        # Output display
        self.output = scrolledtext.ScrolledText(
            self, width=100, height=25, wrap=tk.WORD
        )
        self.output.pack(pady=10)

        # Exit
        tk.Button(self, text="Exit", command=self.quit).pack(pady=10)

    def append_output(self, text):
        self.output.insert(tk.END, text + "\n")
        self.output.see(tk.END)

    def set_cpcon(self):
        try:

            cpcon = self.cpcon_level.get()
            threat = self.threat_type.get()

            if not cpcon or not threat:
                messagebox.showwarning(
                    "Input Required", "Please select both CPCON level and threat type."
                )
                return

            # TO DO: check to see if policy has already been implemented/verified
            cpcon = int(cpcon)
            insert_policy(self.db_connection, cpcon, threat, None, None)
            self.append_output(f"Set CPCON {cpcon} with threat '{threat}'")

            script = CPCON_SCRIPTS.get(cpcon)
            if script:
                try:
                    subprocess.Popen(
                        [
                            "gnome-terminal",
                            "--",
                            "bash",
                            "-c",
                            f"/bin/bash {script}; exit",
                        ],
                        start_new_session=True,
                    )

                    self.append_output(f"Executed script: {script}")
                except Exception as e:
                    self.append_output(f"Script failed: {e}")
            else:
                self.append_output(f"No script defined for CPCON {cpcon}")

        except Exception as e:
            self.append_output(f"Error setting CPCON: {e}")

    def view_hosts(self):
        try:

            hosts = select_all_hosts(self.db_connection)
            if not hosts or hosts == DB_ERROR:
                self.append_output("No hosts found in the database.")
                return

            hosts = sorted(hosts, key=lambda h: ipaddress.ip_address(h["host_ip"]))

            self.append_output("\nHost Modules Table:")
            self.append_output(
                f"{'Host ID':<10}{'Host IP':<15}{'Built Modules':<30}{'Deployed Modules':<30}"
            )
            self.append_output("-" * 95)

            for host in hosts:
                host_id = host["host_id"]
                host_ip = host["host_ip"]

                built_rows = select_all_built_modules(self.db_connection, host_id)
                built_modules = (
                    [
                        row["module"]
                        .replace("MILCOM_isolate", "host_isolate")
                        .replace("MILCOM_server", "server_monitor")
                        for row in built_rows
                    ]
                    if built_rows and built_rows != DB_ERROR
                    else []
                )

                # built_modules = (
                #     [row["module"] for row in built_rows]
                #     if built_rows and built_rows != DB_ERROR
                #     else []
                # )

                deployed_ids = select_deployed_modules(self.db_connection, host_id)
                deployed_modules = []
                if deployed_ids and deployed_ids != DB_ERROR:
                    for mod_id in deployed_ids:
                        mod_row = select_built_module_by_id(
                            self.db_connection, host_id, mod_id
                        )
                        deployed_modules.append(
                            mod_row["module"]
                            .replace("MILCOM_isolate", "host_isolate")
                            .replace("MILCOM_server", "server_monitor")
                            if mod_row and mod_row != DB_ERROR
                            else f"ID:{mod_id}"
                        )

                        # deployed_modules.append(
                        #     mod_row["module"]
                        #     if mod_row and mod_row != DB_ERROR
                        #     else f"ID:{mod_id}"
                        # )

                self.append_output(
                    f"{host_id:<10}{host_ip:<15}{', '.join(built_modules) or 'None':<30}{', '.join(deployed_modules) or 'None':<30}"
                )

        except Exception as e:
            self.append_output(f"Error viewing hosts: {e}")

    def view_policies(self):
        try:
            policies = view_all_policies(self.db_connection)
            if not policies or policies == DB_ERROR:
                self.append_output("No policies found or DB error.")
                return

            self.append_output(f"\n{'CPCON':<10}{'Threat':<25}{'Verified':<10}")
            for p in policies:
                cpcon = p[0]
                threat = p[1]
                verified = p[4] if len(p) > 4 and p[4] is not None else "N/A"
                self.append_output(f"{cpcon:<10}{threat:<25}{verified:<10}")
        except Exception as e:
            self.append_output(f"Error viewing policies: {e}")

    def view_alerts(self):
        try:

            alerts = select_all_alerts(self.db_connection)
            if not alerts or alerts == DB_ERROR:
                self.append_output("No alerts found or DB error.")
                return
            self.append_output(f"\n{'Host ID':<10}{'Host Alerts':<15}")
            for host_id, alert_list in alerts.items():
                for alert in alert_list:
                    self.append_output(f"{host_id:<10}{alert:<15}")
        except Exception as e:
            self.append_output(f"Error viewing alerts: {e}")


if __name__ == "__main__":
    app = NCO_UI()
    app.mainloop()
