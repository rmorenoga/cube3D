
import numpy as np

from path import Path

import matplotlib.pyplot as plt
from scipy.ndimage import zoom
from skimage.measure import block_reduce
from scipy import stats as st
#from mpl_toolkits.mplot3d import axes3d, Axes3D

import binvox
import os
import open3d as o3d
import shutil




def plotCubeAt(pos=(0,0,0),ax=None):
    # Plotting a cube element at position pos
    if ax !=None:
        X, Y, Z = cuboid_data( pos )
        ax.plot_surface(X, Y, Z, color='grey', shade=True, antialiased=True)

def cuboid_data(pos, size=(1,1,1)):
    # code taken from
    # https://stackoverflow.com/a/35978146/4124317
    # suppose axis direction: x: to left; y: to inside; z: to upper
    # get the (left, outside, bottom) point
    o = [a - b / 2 for a, b in zip(pos, size)]
    # get the length, width, and height
    l, w, h = size
    x = [[o[0], o[0] + l, o[0] + l, o[0], o[0]],  
         [o[0], o[0] + l, o[0] + l, o[0], o[0]],  
         [o[0], o[0] + l, o[0] + l, o[0], o[0]],  
         [o[0], o[0] + l, o[0] + l, o[0], o[0]]]  
    y = [[o[1], o[1], o[1] + w, o[1] + w, o[1]],  
         [o[1], o[1], o[1] + w, o[1] + w, o[1]],  
         [o[1], o[1], o[1], o[1], o[1]],          
         [o[1] + w, o[1] + w, o[1] + w, o[1] + w, o[1] + w]]   
    z = [[o[2], o[2], o[2], o[2], o[2]],                       
         [o[2] + h, o[2] + h, o[2] + h, o[2] + h, o[2] + h],   
         [o[2], o[2], o[2] + h, o[2] + h, o[2]],               
         [o[2], o[2], o[2] + h, o[2] + h, o[2]]]               
    return np.array(x), np.array(y), np.array(z)

def plotMatrix(ax, matrix):
    # plot a Matrix 
    for i in range(matrix.shape[0]):
        for j in range(matrix.shape[1]):
            for k in range(matrix.shape[2]):
                if matrix[i,j,k] >0:
                    # to have the 
                    plotCubeAt(pos=(i-0.5,j-0.5,k-0.5), ax=ax) 

    


def write(fname):

  
    with open(fname, 'r+b') as f:
        data  = binvox.read_as_3d_array(f)
	
    data = zoom(data, 0.12, order=2, mode='nearest', cval=0.0, grid_mode=True)
    #data = block_reduce(data, block_size=(10,10,10), func=np.max)
    #data = block_reduce(data, block_size=(3,3,3), func=np.sum)


    #for i in range(round(data.shape[0]/4),round(data.shape[0])):
    #     for j in range(round(data.shape[1])):
    #         for k in range(round(data.shape[2])):
    #             data[i,j,k] = 0

    
    note_size = 0

    for i in range(data.shape[0]):
        for j in range(data.shape[1]):
            for k in range(data.shape[2]):
                if data[i,j,k] >0:
                    note_size = note_size + 1

    print(note_size)

    
    if (np.sum(data)<500):
        ax = plt.figure().add_subplot(projection='3d')
       

        plt.xlabel('x')
        plt.ylabel('y')
        ax.axes.set_xlim3d(left=0, right=20)
        ax.axes.set_ylim3d(bottom=0, top=20) 
        ax.axes.set_zlim3d(bottom=0, top=20)
        plotMatrix(ax, data)
        plt.show() 


    return data



def main():
    return 


if __name__ == '__main__':
  yourpath = 'vox_data/'
  xdata = []
  ydata = []



  for root, dirs, files in os.walk(yourpath, topdown=True):
      for name in files:
          
          if name == "model_normalized.solid.binvox":
            old_path = os.path.join(root, name)
            print(root)
            shutil.copy(old_path, 'model_normalized.solid.binvox')
            data = write(name)
            

            answer = input("Enter yes or no: ")
            
            if answer == "y":
              
              xdata.append(data)
              ydata.append(4)
              np.save('normal_chair_x.npy',xdata)
              np.save('normal_chair_y.npy',ydata)

              
              print(len(xdata))

            if answer == "n":
              break

  print(len(xdata))          

  

          