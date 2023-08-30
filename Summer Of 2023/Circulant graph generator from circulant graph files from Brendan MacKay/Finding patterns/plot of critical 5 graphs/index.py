import numpy as np
import matplotlib.pyplot as plt

x = np.linspace(0, 6, 100)
y = np.sin(x)
z = np.cos(x)

plt.plot(x, y, 'r--', linewidth=3)
plt.plot(x, z, 'k:', linewidth=2)
plt.legend(['y', 'z'])
plt.xlabel('x')
plt.ylabel('values')
plt.xlim([0, 3])
plt.ylim([-1.5, 1.5])
plt.savefig('myFigure.png')
plt.savefig('myFigure.eps')
plt.show()
