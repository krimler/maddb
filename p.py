fp = open('lock')
expected = "LO"
une = "UN"
i =0
f =0 
while 1:
  i += 1
  line = fp.readline()
  if not line:
      break
  if(f == 0):
      assert(expected == line[0: 2]), i
      f = 1
  else:
      assert(une == line[0:2]), i
      f = 0
print("done")
