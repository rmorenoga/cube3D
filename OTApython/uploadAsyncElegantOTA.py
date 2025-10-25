import requests
import hashlib
import optparse
import sys
import random

#try:
from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
from tqdm import tqdm
# except ImportError:
#     env.Execute("$PYTHONEXE -m pip install requests_toolbelt")
#     env.Execute("$PYTHONEXE -m pip install tqdm")
#     from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
#     from tqdm import tqdm

def upload(firmware_path, upload_url,timeOut): 

    with open(firmware_path, 'rb') as firmware:
        md5 = hashlib.md5(firmware.read()).hexdigest()
        firmware.seek(0)
        encoder = MultipartEncoder(fields={
            'MD5': md5, 
            'firmware': ('firmware', firmware, 'application/octet-stream')}
        )

        bar = tqdm(desc='Upload Progress',
              total=encoder.len,
              dynamic_ncols=True,
              unit='B',
              unit_scale=True,
              unit_divisor=1024
              )

        monitor = MultipartEncoderMonitor(encoder, lambda monitor: bar.update(monitor.bytes_read - bar.n))
        
        response = None
        try:
            response = requests.post(upload_url, data=monitor, headers={'Content-Type': monitor.content_type}, timeout=timeOut)
            bar.close()
        except requests.exceptions.Timeout:
          bar.close()
          print(upload_url, "timed out")

        if(response != None):
          print(response,response.text)
        
          
       

def parser(unparsed_args):
  parser = optparse.OptionParser(
    usage = "%prog [options]",
    description = "Transmit image over the air to the esp32 module with OTA support."
  )

  # destination ip and port
  group = optparse.OptionGroup(parser, "Destination")
  group.add_option("-i", "--ip",
    dest = "esp_ip",
    action = "store",
    help = "ESP32 IP Address.",
    default = False
  )
  group.add_option("-I", "--host_ip",
    dest = "host_ip",
    action = "store",
    help = "Host IP Address.",
    default = "0.0.0.0"
  )
  group.add_option("-p", "--port",
    dest = "esp_port",
    type = "int",
    help = "ESP32 ota Port. Default 3232",
    default = 3232
  )
  group.add_option("-P", "--host_port",
    dest = "host_port",
    type = "int",
    help = "Host server ota Port. Default random 10000-60000",
    default = random.randint(10000,60000)
  )
  parser.add_option_group(group)

  # auth
  group = optparse.OptionGroup(parser, "Authentication")
  group.add_option("-a", "--auth",
    dest = "auth",
    help = "Set authentication password.",
    action = "store",
    default = ""
  )
  parser.add_option_group(group)

  # image
  group = optparse.OptionGroup(parser, "Image")
  group.add_option("-f", "--file",
    dest = "image",
    help = "Image file.",
    metavar="FILE",
    default = None
  )
  group.add_option("-s", "--spiffs",
    dest = "spiffs",
    action = "store_true",
    help = "Use this option to transmit a SPIFFS image and do not flash the module.",
    default = False
  )
  parser.add_option_group(group)

  # output group
  group = optparse.OptionGroup(parser, "Output")
  group.add_option("-d", "--debug",
    dest = "debug",
    help = "Show debug output. And override loglevel with debug.",
    action = "store_true",
    default = False
  )
  group.add_option("-t", "--timeout",
    dest = "timeout",
    type = "int",
    help = "Timeout to wait for the ESP32 to accept invitation",
    default = 100
  )
  parser.add_option_group(group)

  (options, args) = parser.parse_args(unparsed_args)

  return options
# end parser

def main(args):
  options = parser(args)
  #upload("./firmware.bin","http://192.168.1.3/update")
  #print(options.image)
  #print("http://"+options.esp_ip+"/update")
  upload(options.image,"http://"+options.esp_ip+"/update",options.timeout)


if __name__ == '__main__':
  sys.exit(main(sys.argv))

