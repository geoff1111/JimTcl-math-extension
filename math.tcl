#! ./jimsh

load ./apex.so

puts [apex {scale=1000; pi(100) * 5 * 5}]
puts [apex {pi(200) * 10 * 10}]
puts [apex {pi(200) * pi(200)}]
puts [apex {pi(300) * pi(300) * pi(300)}]
