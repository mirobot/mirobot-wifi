#!/usr/bin/env ruby

require 'net/http'
require 'json'

def err(msg)
  puts msg
  exit
end


def hex2bin(hexFile)
  bin = []
  File.readlines(hexFile).each do |line|
    # get the length of data
    lineLen = line[1,2].to_i(16)
    # make sure it is a data line
    if lineLen > 0 and line[7,2] == '00'
      # parse the data into an array
      bindata = line[9,2 * lineLen]
      lineLen.times do |i|
        bin << bindata[i*2,2].to_i(16)
      end
    end
  end
  # Pad to a multiple of 4 for reading / writing to flash
  (bin.length % 4).times{ bin << 0 }
  bin.pack('C*')
end

err("usage: flasharduino.rb <address> <hex file>") unless ARGV.length == 2

address = ARGV[0]
hexFile = ARGV[1]

puts "Reading hex file"
fw = hex2bin(hexFile)

puts "Writing to flash"
http = Net::HTTP.new(address)
response = http.post("/admin/updatearduino.cgi", fw) 
err("Couldn't write new data") unless response.code.to_i == 204
puts response.code

puts "Checking firmware"
written = Net::HTTP.get(address, "/admin/readflash.bin?offset=#{0x76000}&length=#{fw.length}")
err("Check failed") unless written = fw

puts "Updating firmware"
http = Net::HTTP.new(address)
response = http.post("/admin/updatearduino.cgi?flash=true", '') 
err("Couldn't flash Arduino") unless response.code.to_i == 204

100.times do
  status = JSON.parse(Net::HTTP.get(address, "/admin/updatearduino.cgi"))
  if status['state'] == 'done'
    puts "Arduino successfully updated"
    exit
  else
    sleep(1)
  end
end