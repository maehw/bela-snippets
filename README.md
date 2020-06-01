# bela-snippets
Code snippets for the bela platform - a real-time, ultra-low-latency audio and sensor processing system



About the snippets (in order of creation):

I've mainly used them until know to check if *Bela* works together with *MaKey MaKey*.

* **scope-kbd:**
  This snippet demonstrates the use of a keyboard plugged into Bela directly.
  Have a look at the Bela Oscilloscope and then type on your keyboard.
  Any key stroke will show as a positive level.
* **sample-kbd:**
  This snippet demonstrates the use of a keyboard plugged into Bela directly.
  As soon as a key (any key) is pushed down, a sound file will be played back.
* **synth-kbd:**
  This snippet is based on the "additive-synth" project of "C++ Real-Time Audio Programming with Bela - Lecture 5: Classes and Objects". It uses an additive synthesiser based on an array of Wavetable oscillator objects. Six different keys are associated with one oscillator each.



Notes:

* There may be dependencies for additional files.

* If there are errors in the keyboard handling code, it's likely that they are in all snippets.
* Feel free to contribute or fork.