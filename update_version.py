from datetime import datetime
import os

# 獲取腳本所在目錄
base_dir = os.path.dirname(os.path.abspath(__file__))

# 設定相對路徑
include_dir = os.path.join(base_dir, 'examples', 'das_subg_thread', 'das_subg_thread', 'Include')
version_txt_path = os.path.join(include_dir, 'version.txt')
version_h_path = os.path.join(include_dir, 'version.h')

# Read the current version number from version.txt
with open(version_txt_path, "r") as f:
    version = f.read().strip()

with open(version_h_path, "r") as f:
    version_h_content = f.read()
    print("Old version: {0}".format(version_h_content))

# Update the version number
version = "RM"  # increment the version number

# Get the current date and time
now = datetime.now()

# Format the date and time
date = now.strftime("%Y") + '/' + str(int(now.strftime("%m"))) + '/' + str(int(now.strftime("%d")))
time = now.strftime("%H:%M:%S")

# 在計算新版本號後打印它
print("New version: #define VERSION \"{0} {1} {2}\"".format(version, date, time))

# Write the updated version number and current date and time to version.h
with open(version_h_path, "w") as f:
    f.write(f'#define VERSION "{version} {date} {time}"\n')

# Write the updated version number to version.txt
with open(version_txt_path, "w") as f:
    f.write(version)  # write the entire version string, including "RMv"
