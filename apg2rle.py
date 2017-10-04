# Based on the script written by LegionMammal978 and zyabin101 (https://chat.stackexchange.com/transcript/42936?m=31253844#31253844).


import golly

def readApg(code):
  try:
    [prefix, data] = code.split('_', 1)
  except ValueError:
    raise Exception('Invalid apgcode {}'.format(invalid))
  if prefix == 'ov':
    try:
      data = data.split('_')[1]
    except ValueError:
      raise Exception('Invalid apgcode {}'.format(invalid))
  chars = '0123456789abcdefghijklmnopqrstuv'
  i = 0
  while i < len(data):
    if data[i] in chars or data[i] in ('w', 'x', 'z'):
      i += 1
    elif data[i] == 'y':
      if i + 1 == len(data) or not(data[i + 1] in chars):
        raise Exception('Invalid apgcode {}'.format(invalid))
      i += 2
    else:
      raise Exception('Invalid apgcode {}'.format(invalid))
  cursorX, cursorY, i = 0, 0, 0
  cellList = []
  while i < len(data):
    if data[i] == 'w':
      cursorX += 2
    elif data[i] == 'x':
      cursorX += 3
    elif data[i] == 'y':
      cursorX += chars.find(data[i + 1]) + 4
      i += 1
    elif data[i] == 'z':
      cursorX = 0
      cursorY += 5
    else:
      col = chars.find(data[i])
      if col & 1 != 0: cellList += [cursorX, cursorY]
      if col & 2 != 0: cellList += [cursorX, cursorY + 1]
      if col & 4 != 0: cellList += [cursorX, cursorY + 2]
      if col & 8 != 0: cellList += [cursorX, cursorY + 3]
      if col & 16 != 0: cellList += [cursorX, cursorY + 4]
      cursorX += 1
    i += 1
  return cellList

filename = golly.opendialog('Choose an apgsearch log file')
dirname = golly.opendialog('Choose a folder for the output', 'dir')
with open(filename) as f:
  line = f.readline()
  while line:
    if not line.isspace() and line[0] != '@':
      code = line.split()[0]
      try:
        cellList = readApg(code)
      except Exception, args:
        golly.warn(args[0])
      else:
        golly.store(cellList, dirname + code + '.rle')
    line = f.readline()
