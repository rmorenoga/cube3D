# Project Cube 3D

## Training Data:

The training data is downloaded from https://www.shapenet.org/ as ModelShapeNetCore, model_normalized.solid.binvox format. We use 7 classes: aircraft, boat, car, guitar, birdhouse(house), table and chair.

This data is then reduced by the function “training_data_cleaning.py”. Running this file will present you with scaled down versions of the downloaded training data. If you think the image is acceptable, close the image popup, and write “y” into the terminal. Else, write “n” in the terminal. The terminal will display how many successful images have been saved and also the number of voxels in each piece of data. Note: the data is expected to be in the file subfolder location vox_data/…. Cleaning the data is done one class at a time, and labels are saved simultaneously with the xdata. To change the class type, change line 129. 

The classification output should be as follows:

**N.  Class   Color**
- 0 **Plane** green  
- 1 **Chair** blue  
- 2 **Car** purple  
- 3 **Table** yellow 
- 4 **House** white  
- 5 **Guitar** pink  
- 6 **Boat** cyan
- **Damaged** red


The code will output two .npy files xdata and ydata that are ready to be inputted to the neural network training code. **Already cleaned data is stored in the xdata_7class.npy and ydata_7class.npy**

## Training, Simulation and graphs

Training and simulation are performed using python code organized in a jupyter notebook that runs preferably in Google colab **BIGCUBE_V2.ipynb**. To open it you can either clone this repository and upload the notebook to Google colab, or create a fork of the repository and open it directly from github using Google colab. The notebook also contains functions for exporting the model weights as a file for compiling the microcontroller code, tools for checking connections between real cubes and tools for generating graphs. For further instructions check the different cell descriptions inside the notebook. 

## Training the neural network:

To train the neural network I use a google colab notebook. The first notebook cell is responsible for training the data. It reads the xdata.npy and ydata.npy files created using the process in the previous section. N.B for this notebook to run you also need nca3d_v1.py in the same folder.

Once the first cell of the notebook in completed, the model is saved in models/dropout.


## Simulation of cubes 
The next cell of the notebook simulated how the cells would perform in hardware. This is then visualised in the cell below. The cells change colour with respect to what cell they think they are a part of. See table above for the colours.
This cell also outputs an array of the percentage of correctly identified cells, which can be used for graph plotting :) there’s also a loop function, where you can run each shape several times. Because of the randomness of the cell updates sometimes the results vary.

## Outputting neural network for firmware
To output the neural network to be uploaded to the firmware, run the corresponding cell in the jupyter notebook. It produces a text file that can be copied directly into the neural_network.h required by the firmware.


## Cube firmware

The cube firmware is saved in the 3DCubePlatformIO folder. You also need the neural_network.h file located in the root file of the repository. The following steps have been tested on Windows 10 and 11.

### Compiling
To be able to upload the firmware to the cubes it should first be compiled. The 3DCubePlatformIO folder is set up and configured as a [platformIO](platformio.org/) project. To make things easier we reccomend opening the folder with [visual studio code](https://code.visualstudio.com/). VSCode will automatically detect the platformio.ini file in the repository and ask you to install the platformio extension. The extension will then automatically donwload all the required compilers and libraries. After installation press the compile button (check mark at the bottom left corner of VSCode). This will generate the firmware.bin file in the 3DCubePlatformIO/.pio/build/adafruit_metro_esp32s2/ subfolder. 

### Uploading
To upload the firmware.bin file first make sure the cubes are connected to the same local network. Then navigate to the OTApython folder and run the ESPuploadParallel.py script. This script should upload the firmware to all cubes connected in the network. To change the subnet that is scanned by the script, change the address ranges in lines 18, 80, 86, 52 and 53.

### Getting data from the cubes

After all cubes have been programmed. Turn them off, navigate to the StateReceivers and launch the **docker-compose.yaml** server. This requires that you have [docker](docker.com/) installed in your system and assumes that you have a [mongodb](mongodb.com/) installation with a **Test** database and a **Dummy** collection set up. Additionally make sure that the computer where you launch the server is on IP 192.168.72.22 and that you have firewall permissions to receive on TCP port 8000. You can change the server address in the firmware (/3DCubePlatformIO/src/3d_cube.ino) in line 688, this needs the cubes to be reprogrammed. After you launch the server, turn on the assembled cubes. The cubes will first perform the classification process, then connect to the network specified in lines 602 and 603 in the firmware (/3DCubePlatformIO/src/3d_cube.ino) and then post the data to the server.

After the data from all cubes arrive to the server, you can export the collection as a .json file.

 

