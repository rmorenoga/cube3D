import tensorflow as tf
import numpy as np
from tensorflow.keras.layers import Conv3D


class NCA3D(tf.keras.Model):
    def __init__(self, num_hidden_channels: int = 20, num_classes: int = 7, alpha_living_threshold: float = 0.1, cell_fire_rate: float = 0.5):
        super().__init__()
        self.num_classes = num_classes
        self.num_hidden_channels = num_hidden_channels
        self.alpha_living_threshold = alpha_living_threshold
        self.cell_fire_rate = cell_fire_rate

        self.channel_n = num_hidden_channels + num_classes + 1
        self.perception_channels = self.channel_n * 3
        self.kernel_mask = np.array(
            [[[0, 0, 0],
              [0, 1, 0],
              [0, 0, 0]],

             [[0, 1, 0],
              [1, 1, 1],
              [0, 1, 0]],

             [[0, 0, 0],
              [0, 1, 0],
              [0, 0, 0]],
             ])[:, :, :, np.newaxis, np.newaxis]

        self.perceive = tf.keras.Sequential([
            Conv3D(self.perception_channels, 3, activation=tf.nn.relu, padding="SAME"),  # (c, 3, 3, 80)
        ])

        self.dmodel = tf.keras.Sequential([
            Conv3D(self.perception_channels, 1, activation=tf.nn.relu),  # (80, 1, 1, 80)
            Conv3D(self.channel_n - 1, 1, kernel_initializer=tf.zeros_initializer),  # (80, 1, 1, c)
        ])

        self(tf.zeros([1, 4, 4, 4, self.channel_n]))  # dummy calls to build the model
        self.reset_diag_kernel()

    def reset_diag_kernel(self):
        kernel, bias = self.perceive.layers[0].get_weights()
        self.perceive.layers[0].set_weights([kernel * self.kernel_mask, bias])

    @tf.function
    def call(self, x):
        gray, state = tf.split(x, [1, self.channel_n - 1], -1)
        update = self.dmodel(self.perceive(x))
        update_mask = tf.random.uniform(tf.shape(x[:, :, :, :, :1])) <= self.cell_fire_rate
        living_mask = gray > self.alpha_living_threshold
        residual_mask = tf.cast(update_mask & living_mask, tf.float32)
        
        state += residual_mask * tf.nn.tanh(update)
        
       
        return tf.concat([gray, state], -1)

    @tf.function
    def classify(self, x):
        # The last 10 layers are the classification predictions, one channel
        # per class. Keep in mind there is no "background" class,
        # and that any loss doesn't propagate to "dead" pixels.
        return x[:, :, :, :, -self.num_classes:]

    @tf.function
    def initialize(self, structure):
        shape = structure.shape
        state = tf.zeros([shape[0], shape[1], shape[2], shape[3], self.channel_n - 1])  # (bs, 4, 3, 3, n-1)
        structure = tf.reshape(structure, [shape[0], shape[1], shape[2], shape[3], 1])  # (bs, 4, 3, 3, 1)
        return tf.concat([structure, state], -1)  # (bs, 4, 3, 3, n)
