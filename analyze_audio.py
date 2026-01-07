#!/usr/bin/env python3
"""
Audio diagnostic script for Modal Attractors test output
Analyzes test_output.wav for common audio issues
"""

import wave
import struct
import numpy as np
import sys

def analyze_wav(filename):
    print(f"Analyzing: {filename}")
    print("=" * 60)

    try:
        with wave.open(filename, 'r') as wav:
            # Get WAV parameters
            channels = wav.getnchannels()
            sample_width = wav.getsampwidth()
            framerate = wav.getframerate()
            n_frames = wav.getnframes()

            print(f"Channels: {channels}")
            print(f"Sample width: {sample_width} bytes ({sample_width * 8} bit)")
            print(f"Sample rate: {framerate} Hz")
            print(f"Total frames: {n_frames}")
            print(f"Duration: {n_frames / framerate:.2f} seconds")
            print()

            # Read all frames
            frames = wav.readframes(n_frames)

            # Convert to numpy array (16-bit PCM)
            if sample_width == 2:
                samples = np.array(struct.unpack(f'<{n_frames * channels}h', frames), dtype=np.float32)
            else:
                print(f"Unsupported sample width: {sample_width}")
                return

            # Normalize to [-1.0, 1.0]
            samples = samples / 32768.0

            # Split stereo channels
            if channels == 2:
                left = samples[0::2]
                right = samples[1::2]
            else:
                left = samples
                right = samples

            # Analysis
            print("AUDIO ANALYSIS")
            print("=" * 60)

            # 1. Amplitude statistics
            print(f"\nAmplitude Statistics (Left Channel):")
            print(f"  RMS:  {np.sqrt(np.mean(left**2)):.6f}")
            print(f"  Peak: {np.max(np.abs(left)):.6f}")
            print(f"  Min:  {np.min(left):.6f}")
            print(f"  Max:  {np.max(left):.6f}")

            # 2. DC offset
            dc_offset = np.mean(left)
            print(f"\n  DC Offset: {dc_offset:.6f}")
            if abs(dc_offset) > 0.001:
                print(f"  ⚠️  WARNING: Significant DC offset detected!")

            # 3. Check for clipping
            clipped_samples = np.sum(np.abs(left) >= 0.99)
            print(f"\n  Clipped samples: {clipped_samples} ({clipped_samples/len(left)*100:.3f}%)")
            if clipped_samples > 0:
                print(f"  ⚠️  WARNING: Audio is clipping!")

            # 4. Check for discontinuities (clicks/pops)
            diff = np.diff(left)
            max_diff = np.max(np.abs(diff))
            click_threshold = 0.1  # Sudden jumps > 10% of full scale
            clicks = np.sum(np.abs(diff) > click_threshold)
            print(f"\n  Max sample difference: {max_diff:.6f}")
            print(f"  Potential clicks (diff > {click_threshold}): {clicks}")
            if clicks > 10:
                print(f"  ⚠️  WARNING: Many discontinuities detected!")
                # Show first few clicks
                click_indices = np.where(np.abs(diff) > click_threshold)[0][:5]
                for idx in click_indices:
                    print(f"      Click at sample {idx}: {diff[idx]:.6f}")

            # 5. Check for NaN or Inf
            nan_count = np.sum(np.isnan(left))
            inf_count = np.sum(np.isinf(left))
            print(f"\n  NaN samples: {nan_count}")
            print(f"  Inf samples: {inf_count}")
            if nan_count > 0 or inf_count > 0:
                print(f"  ⚠️  ERROR: Invalid samples detected!")

            # 6. Zero crossing rate (roughness indicator)
            zero_crossings = np.sum(np.diff(np.sign(left)) != 0) / 2
            zcr = zero_crossings / n_frames * framerate
            print(f"\n  Zero crossing rate: {zcr:.1f} Hz")

            # 7. Spectral analysis (simple)
            print(f"\n  Computing frequency spectrum...")
            fft = np.fft.rfft(left[:min(4096, len(left))])
            freqs = np.fft.rfftfreq(min(4096, len(left)), 1/framerate)
            magnitude = np.abs(fft)

            # Find peaks
            peak_indices = np.argsort(magnitude)[-5:][::-1]
            print(f"\n  Top 5 frequency components:")
            for i, idx in enumerate(peak_indices):
                if magnitude[idx] > 0.01:  # Only show significant peaks
                    print(f"    {i+1}. {freqs[idx]:.1f} Hz (magnitude: {magnitude[idx]:.3f})")

            # 8. Silence detection
            silence_threshold = 0.001
            silent_samples = np.sum(np.abs(left) < silence_threshold)
            print(f"\n  Silent samples (<{silence_threshold}): {silent_samples} ({silent_samples/len(left)*100:.1f}%)")

            # 9. Check envelope decay (for resonator)
            print(f"\n  Envelope Analysis:")
            window_size = framerate // 10  # 100ms windows
            num_windows = len(left) // window_size
            envelope = []
            for i in range(num_windows):
                window = left[i*window_size:(i+1)*window_size]
                rms = np.sqrt(np.mean(window**2))
                envelope.append(rms)

            if len(envelope) > 1:
                # Check if decaying
                is_decaying = envelope[0] > envelope[-1]
                decay_rate = (envelope[0] - envelope[-1]) / envelope[0] if envelope[0] > 0 else 0
                print(f"    Initial RMS: {envelope[0]:.6f}")
                print(f"    Final RMS:   {envelope[-1]:.6f}")
                print(f"    Decay rate:  {decay_rate*100:.1f}%")
                if is_decaying:
                    print(f"    ✓ Envelope is decaying (resonator behavior)")
                else:
                    print(f"    ⚠️  Envelope not decaying (unexpected for resonator)")

            print()
            print("=" * 60)
            print("OVERALL ASSESSMENT")
            print("=" * 60)

            issues = []
            if abs(dc_offset) > 0.001:
                issues.append("DC offset")
            if clipped_samples > 0:
                issues.append("Clipping")
            if clicks > 10:
                issues.append("Clicks/pops")
            if nan_count > 0 or inf_count > 0:
                issues.append("Invalid samples (NaN/Inf)")
            if np.sqrt(np.mean(left**2)) < 0.001:
                issues.append("Too quiet")

            if issues:
                print(f"⚠️  Issues detected: {', '.join(issues)}")
            else:
                print(f"✓ Audio appears clean")

            print()

    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
    except Exception as e:
        print(f"Error analyzing file: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    filename = sys.argv[1] if len(sys.argv) > 1 else "test_output.wav"
    analyze_wav(filename)
