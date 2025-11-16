<h1 align="center">🎥 Video Stabilization using OpenCV</h1>

<p>
This repository contains a C++ project that stabilizes shaky video footage using OpenCV.  
It implements a common video stabilization algorithm based on estimating motion, smoothing the motion trajectory, and applying transformations to counteract the shake.
</p>

<hr>

<h2>📦 Project Components</h2>

<ul>
  <li><strong>stabilize</strong>: Takes a shaky video as input and produces a stabilized video.</li>
  <li><strong>main (compare utility)</strong>: Creates a side-by-side comparison video showing original vs stabilized footage.</li>
</ul>

<hr>

<h2>🧠 Core Stabilization Logic</h2>

<ol>
  <li>
    <strong>Estimate Motion</strong>:  
    For each pair of consecutive frames:
    <ul>
      <li>Detects feature points (<code>goodFeaturesToTrack</code>).</li>
      <li>Tracks points using optical flow (<code>calcOpticalFlowPyrLK</code>).</li>
      <li>Computes rigid transform using <code>estimateAffinePartial2D</code>.</li>
    </ul>
  </li>

  <li>
    <strong>Accumulate Trajectory</strong>:  
    Inter-frame transforms are accumulated to obtain the camera's overall motion trajectory (dx, dy, da).
  </li>

  <li>
    <strong>Smooth Trajectory</strong>:  
    A moving average filter removes high-frequency jitter.  
    The smoothing intensity is controlled by <code>SMOOTHING_RADIUS</code>.
  </li>

  <li>
    <strong>Generate New Transforms</strong>:  
    The difference between original and smoothed trajectories—the "shake"—is removed by adjusting transforms.
  </li>

  <li>
    <strong>Apply Transforms</strong>:  
    Each frame is warped using <code>warpAffine</code> to create the final stable video  
    (<code>stabilized_output.avi</code>).
  </li>
</ol>

<hr>

<h2>🛠 Dependencies</h2>

<ul>
  <li>C++ Compiler (g++, Clang, etc.)</li>
  <li>CMake (3.10+)</li>
  <li>OpenCV (core, videoio, imgproc, highgui, video)</li>
</ul>

<hr>

<h2>⚙️ How to Build</h2>

<pre>
git clone https://github.com/nachiketagrawal2310/Video-stabilization-using-opencv.git
cd Video-stabilization-using-opencv
</pre>

<p><strong>Create build directory:</strong></p>

<pre>
mkdir build
cd build
</pre>

<p><strong>Run CMake and Make:</strong></p>

<pre>
cmake ..
make
</pre>

<p>This generates two executables: <code>stabilize</code> and <code>main</code>.</p>

<hr>

<h2>▶️ How to Use</h2>

<h3>Step 1: Stabilize a Video</h3>

<pre>
./stabilize /path/to/your/shaky_video.mp4
</pre>

<p>
Outputs: <code>stabilized_output.avi</code>
</p>

<h3>Step 2: Compare Original and Stabilized Videos</h3>

<pre>
./main /path/to/your/shaky_video.mp4 stabilized_output.avi
</pre>

<p>
Outputs: <code>before_after_output.avi</code> (side-by-side comparison)
</p>

<hr>

<h2>📄 License</h2>

<p>
This project is open-sourced under the MIT License.  
(Add a <code>LICENSE</code> file to your repository if you haven't already.)
</p>
