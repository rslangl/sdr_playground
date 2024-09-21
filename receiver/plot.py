import numpy as np
import matplotlib.pyplot as plt

data = np.fromfile('build/samples.cs16', dtype=np.int16)

iq_data = data.reshape(-1, 2)

complex_samples = iq_data[:, 0] + 1j * iq_data[:, 1]

sample_rate = 10e6
N = len(complex_samples)
fft_data = np.fft.fft(complex_samples)

fft_data_shifted = np.fft.fftshift(fft_data)

freqs = np.fft.fftfreq(N, 1/sample_rate)
freqs_shifted = np.fft.fftshift(freqs)

plt.figure(figsize=(10, 0))

plt.plot(freqs_shifted, 20 * np.log10(np.abs(fft_data_shifted)), color='blue')

plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude (dB)')
plt.grid()
plt.legend()

plt.show()
