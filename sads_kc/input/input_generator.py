import sys
import random

if len(sys.argv) != 3:
	print 'usage> python input_generator.py <number of ip addresses> <number of visits>'
	sys.exit(-1)


num_ip = int(sys.argv[1])
num_visit = int(sys.argv[2])


ip_list = []
for i in range(0, num_ip):
	ip = ''
	for j in range(0, 4):
		ip += str(random.randint(0, 255))
		if j != 3:
			ip += '.'
	#end for

	ip_list.append(ip)
#end for

file = open('sads_input_%s_%s.dat' % (str(num_ip), str(num_visit)), 'w')

file.write('%s\n' % num_visit)
for i in range(0, num_visit):
	file.write('%s\n' % ip_list[random.randint(0, len(ip_list)-1)])
