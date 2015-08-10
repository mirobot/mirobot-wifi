#!/usr/bin/env ruby

require 'net/http'
require 'json'

def err(msg)
  puts msg
  exit
end


err("No address specified") unless ARGV.length == 1

address = ARGV[0]

romSlot = JSON.parse(Net::HTTP.get(address, '/admin/settings.json'))['romSlot']
err("Bad rom slot") unless ["0", "1"].include?(romSlot)
newSlot = romSlot == '0' ? '1' : '0'
fwFile = "firmware/mirobot-v2-wifi.rom#{newSlot}.bin"

puts "Writing firmware to slot #{newSlot}"
http = Net::HTTP.new(address)
fw = File.open(fwFile, 'rb') { |f| f.read }
response = http.post("/admin/updatewifi.cgi", fw) 
err("Couldn't write new data") unless response.code.to_i == 204

puts "Checking firmware"
offset = newSlot == 0 ? 0x02000 : 0x39000 
written = Net::HTTP.get(address, "/admin/readflash.bin?offset=#{offset}&length=#{fw.length}")
err("Check failed") unless written = fw

puts "Switching firmware"
http = Net::HTTP.new(address)
response = http.post("/admin/updatewifi.cgi?commit=true", '') 
err("Couldn't switch") unless response.code.to_i == 204
