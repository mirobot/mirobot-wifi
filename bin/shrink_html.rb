#!/usr/bin/env ruby

require 'nokogiri'
require 'uglifier'
require 'cssminify'

SOURCE_DIR = 'web'
OUTPUT_DIR = 'html'

`rm -r #{OUTPUT_DIR}` if Dir.exist?(OUTPUT_DIR) 
Dir.mkdir('html')

def squish_html(input)
  doc = Nokogiri::HTML(input)
  # Compress and combine all of the inline scripts
  scripts = []
  doc.xpath('//script').each do |s|
    src = s['src'] ? File.read("#{SOURCE_DIR}/#{s['src']}") : s.content
    scripts << Uglifier.new({:comments => :none}).compile(src)
    s.remove
  end
  # re-insert the js at the end
  node = Nokogiri::XML::Node.new('script',doc)
  node.content = scripts.join("\n")
  doc.xpath('//body').children.after(node)
  # Remove whitespace from the html
  doc.xpath('//text()').each do |node|
    node.remove unless node.content=~/\S/
  end
  # Minify the CSS
  doc.xpath('//style').each do |c|
    c.content = CSSminify.compress(c.content)
  end
  doc.to_s#.gsub(">\n", '>')
end

def process_file(f)
  if f =~ /\.html/
    content = File.read(f)
    compressed = squish_html(content)
    File.write(f.gsub(SOURCE_DIR, OUTPUT_DIR), compressed)
  else
    `cp #{f} #{f.gsub(SOURCE_DIR, OUTPUT_DIR)}`
  end
end

def process_dir(dir)
  Dir.glob(dir + '/*').each do |f|
    next if f =~ /\.js/ || f =~ /\.css/
    if File.directory?(f)
      Dir.mkdir(f.gsub(SOURCE_DIR, OUTPUT_DIR))
      process_dir(f)
      next
    end
    process_file(f)
  end
end

process_dir(SOURCE_DIR)