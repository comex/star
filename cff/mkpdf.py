import zlib
u = open('out.cff').read()
z = zlib.compress(u)
pdf = open('out.pdf.template').read()
pdf = pdf.replace('XXX', z).replace('YYY', str(len(z)))#'x\x01' + z)#[2:-4]
open('out.pdf', 'w').write(pdf)

