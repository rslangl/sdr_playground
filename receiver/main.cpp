#include <SoapySDR/Constants.h>
#include <SoapySDR/Formats.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Types.hpp>

#include <fftw3.h>

#include <fstream>
#include <map>
#include <string>
#include <vector>

#define NUM_SAMPLES 1024
#define SAMPLE_RATE 10e6

static auto calculate_noise_floor(std::vector<double> &magnitudes) -> double {
  std::sort(magnitudes.begin(), magnitudes.end());
  size_t mid = magnitudes.size() / 2;
  if (magnitudes.size() % 2 == 0) {
    return (magnitudes[mid - 1] + magnitudes[mid]) / 2.0;
  } else {
    return magnitudes[mid];
  }
}

static auto peak_detection(fftw_complex *samples,
                           const int num_samples) -> std::vector<double> {
  std::vector<double> magnitudes(num_samples);
  std::vector<double> peak_frequencies;

  for (int i = 0; i < num_samples; ++i) {
    double real = samples[i][0];
    double imag = samples[i][1];
    magnitudes[i] = sqrt(real * real + imag * imag);
  }

  double noise_floor = calculate_noise_floor(magnitudes);

  double threshold = noise_floor * pow(10, 10.0 / 20.0);

  for (int i = 1; i < num_samples - 1; ++i) {
    if (magnitudes[i - 1] < threshold) {
      magnitudes[i - 1] = 0;
    }

    if (magnitudes[i] > magnitudes[i - 1] &&
        magnitudes[i] > magnitudes[i + 1]) {
      double freq = (double)i / NUM_SAMPLES * SAMPLE_RATE;
      peak_frequencies.push_back(freq);
    }
  }
  return peak_frequencies;
}

static auto write_file(std::complex<int16_t> *buffer,
                       const int num_samples) -> int {
  std::ofstream outfile("samples.cs16", std::ios::out | std::ios::binary);
  if (!outfile) {
    fprintf(stderr, "Could not open file for write\n");
    return -1;
    // SoapySDR::Device::unmake(sdr);
    // return EXIT_FAILURE;
  }
  for (int i = 0; i < num_samples; ++i) {
    outfile.write(reinterpret_cast<char *>(&buffer[i]),
                  NUM_SAMPLES * sizeof(std::complex<int16_t>));
  }
  outfile.close();
  return 0;
}

auto main() -> int {

  // Get device
  SoapySDR::KwargsList results = SoapySDR::Device::enumerate();
  SoapySDR::Kwargs::iterator it;

  for (int i = 0; i < results.size(); ++i) {
    printf("Found device: #%d: ", i);
    for (it = results[i].begin(); it != results[i].end(); ++it) {
      printf("%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }

  int user_selection = -1;
  printf("Select device: ");
  scanf("%d", &user_selection);
  if (user_selection < 0 || user_selection > results.size()) {
    fprintf(stderr, "Not a valid device selection\n");
    return EXIT_FAILURE;
  }

  SoapySDR::Kwargs args = results[user_selection];
  SoapySDR::Device *sdr = SoapySDR::Device::make(args);
  if (sdr == nullptr) {
    fprintf(stderr, "SoapySDR::Device::make failed\n");
    return EXIT_FAILURE;
  }
  user_selection = -1;

  // Query device info
  std::vector<std::string> str_list;

  // See:
  // https://wiki.myriadrf.org/LimeSDR-Mini_v1.1_hardware_description#RF_Frequency_Range
  // RX1_H RF input for 2GHz - 3.5GHz
  // RX1_W RF input for 10MHz - 2GHz
  // Antennas:
  // LNAL: 0.1 MHz - 2000 MHz
  // LNAH: 1500 MHz - 3800 MHz
  // LNAW: 0.1 - 3800 MHz
  str_list = sdr->listAntennas(SOAPY_SDR_RX, 0);
  std::map<int, std::string> antennas{};
  printf("Rx antennas: \n");
  for (int i = 0; i < str_list.size(); ++i) {
    antennas.insert({i, str_list[i]});
    printf("#%d: %s\n", i, str_list[i].c_str());
  }
  printf("\n");

  printf("Select antenna: ");
  scanf("%d", &user_selection);
  if (user_selection < 0 || user_selection > str_list.size()) {
    fprintf(stderr, "Not a valid antenna selection\n");
    return EXIT_FAILURE;
  }
  std::string antenna = antennas[user_selection];
  printf("Using antenna: %s\n", antenna.c_str());
  sdr->setAntenna(SOAPY_SDR_RX, 0, antenna);

  str_list = sdr->listGains(SOAPY_SDR_RX, 0);
  printf("Rx gains: ");
  for (int i = 0; i < str_list.size(); ++i) {
    printf("%s, ", str_list[i].c_str());
  }
  printf("\n");

  SoapySDR::RangeList ranges = sdr->getFrequencyRange(SOAPY_SDR_RX, 0);
  printf("Rx frequency ranges: ");
  for (int i = 0; i < ranges.size(); ++i) {
    printf("[%g Hz -> %g Hz], ", ranges[i].minimum(), ranges[i].maximum());
  }
  printf("\n");

  // Apply tuner settings
  sdr->setSampleRate(SOAPY_SDR_RX, 0, 10e6);
  sdr->setGain(SOAPY_SDR_RX, 0, "DC", 1.0);
  sdr->setGain(SOAPY_SDR_RX, 0, "LNA", 30);
  sdr->setGain(SOAPY_SDR_RX, 0, "TIA", 15);
  sdr->setGain(SOAPY_SDR_RX, 0, "PGA", 10);
  sdr->setFrequency(SOAPY_SDR_RX, 0, 865100000);

  SoapySDR::Stream *rx_stream = sdr->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CS16);
  if (rx_stream == nullptr) {
    fprintf(stderr, "Rx stream setup failed\n");
    SoapySDR::Device::unmake(sdr);
    return EXIT_FAILURE;
  }
  sdr->activateStream(rx_stream, 0, 0, 0);

  std::complex<int16_t> buff[NUM_SAMPLES];
  fftw_complex *in = fftw_alloc_complex(NUM_SAMPLES);
  fftw_complex *out = fftw_alloc_complex(NUM_SAMPLES);
  fftw_plan plan =
      fftw_plan_dft_1d(NUM_SAMPLES, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

  std::vector<double> peak_frequencies;

  for (int i = 0; i < 100; ++i) {
    void *buffs[] = {buff};
    int flags;
    long long time_ns;
    int ret =
        sdr->readStream(rx_stream, buffs, NUM_SAMPLES, flags, time_ns, 1e5);

    if (ret > 0) {
      printf("ret = %d, flags = %d, time_ns = %lld\n", ret, flags, time_ns);

      for (int i = 0; i < ret; ++i) {
        in[i][0] = static_cast<double>(buff[i].real());
        in[i][1] = static_cast<double>(buff[i].imag());
      }

      fftw_execute(plan);

      peak_frequencies = peak_detection(out, NUM_SAMPLES);
      for (auto &frq : peak_frequencies) {
        printf("Peak detected: %f\n", frq);
      }

      write_file(buff, NUM_SAMPLES);
    }
  }

  sdr->deactivateStream(rx_stream, 0, 0);
  sdr->closeStream(rx_stream);

  fftw_destroy_plan(plan);
  fftw_free(out);
  fftw_free(in);

  SoapySDR::Device::unmake(sdr);
  printf("Done\n");

  return EXIT_SUCCESS;
}
