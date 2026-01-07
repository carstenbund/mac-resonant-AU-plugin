#!/usr/bin/env python3
"""
Simple audio diagnostic for Modal Attractors test output
Uses only Python standard library
"""

import wave
import struct
import math
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

            # Convert to samples (16-bit PCM)
            if sample_width != 2:
                print(f"Unsupported sample width: {sample_width}")
                return

            samples = []
            for i in range(0, len(frames), 2):
                sample = struct.unpack('<h', frames[i:i+2])[0]
                samples.append(sample / 32768.0)  # Normalize to [-1, 1]

            # Split stereo
            left = samples[0::2]
            right = samples[1::2]

            print("AUDIO ANALYSIS")
            print("=" * 60)

            # Calculate statistics
            sum_sq = sum(x*x for x in left)
            rms = math.sqrt(sum_sq / len(left))
            peak = max(abs(x) for x in left)
            min_val = min(left)
            max_val = max(left)
            mean_val = sum(left) / len(left)

            print(f"\nAmplitude Statistics (Left Channel):")
            print(f"  RMS:  {rms:.6f}")
            print(f"  Peak: {peak:.6f}")
            print(f"  Min:  {min_val:.6f}")
            print(f"  Max:  {max_val:.6f}")
            print(f"  DC Offset: {mean_val:.6f}")

            if abs(mean_val) > 0.001:
                print(f"  ⚠️  WARNING: Significant DC offset!")

            # Check for clipping
            clipped = sum(1 for x in left if abs(x) >= 0.99)
            print(f"\n  Clipped samples: {clipped} ({clipped/len(left)*100:.3f}%)")
            if clipped > 0:
                print(f"  ⚠️  WARNING: Audio is clipping!")

            # Check for large discontinuities
            max_jump = 0
            large_jumps = 0
            jump_threshold = 0.1
            for i in range(1, len(left)):
                diff = abs(left[i] - left[i-1])
                if diff > max_jump:
                    max_jump = diff
                if diff > jump_threshold:
                    large_jumps += 1

            print(f"\n  Max sample jump: {max_jump:.6f}")
            print(f"  Large jumps (>{jump_threshold}): {large_jumps}")
            if large_jumps > 10:
                print(f"  ⚠️  WARNING: Many discontinuities (clicks/pops)!")

            # Check for silence
            silent = sum(1 for x in left if abs(x) < 0.001)
            print(f"\n  Silent samples (<0.001): {silent} ({silent/len(left)*100:.1f}%)")

            # Check envelope decay
            window_size = framerate // 10  # 100ms windows
            num_windows = len(left) // window_size
            if num_windows > 1:
                first_window_rms = math.sqrt(sum(left[i]**2 for i in range(window_size)) / window_size)
                last_start = (num_windows - 1) * window_size
                last_window_rms = math.sqrt(sum(left[i]**2 for i in range(last_start, last_start + window_size)) / window_size)

                print(f"\n  Envelope Analysis:")
                print(f"    First 100ms RMS: {first_window_rms:.6f}")
                print(f"    Last 100ms RMS:  {last_window_rms:.6f}")

                if first_window_rms > last_window_rms:
                    decay = (first_window_rms - last_window_rms) / first_window_rms * 100
                    print(f"    Decay: {decay:.1f}%")
                    print(f"    ✓ Envelope decaying (resonator behavior)")
                else:
                    print(f"    ⚠️  Envelope not decaying")

            # Summary
            print()
            print("=" * 60)
            print("ISSUES DETECTED:")
            print("=" * 60)

            issues_found = False
            if abs(mean_val) > 0.001:
                print("  ⚠️  DC offset present")
                issues_found = True
            if clipped > 0:
                print("  ⚠️  Clipping detected")
                issues_found = True
            if large_jumps > 10:
                print("  ⚠️  Discontinuities (clicks/pops)")
                issues_found = True
            if rms < 0.001:
                print("  ⚠️  Signal too quiet")
                issues_found = True

            if not issues_found:
                print("  ✓ No major issues detected")

            print()

    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    filename = sys.argv[1] if len(sys.argv) > 1 else "test_output.wav"
    analyze_wav(filename)
