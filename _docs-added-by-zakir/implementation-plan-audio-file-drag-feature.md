# Implementation Plan: Playable Audio Files Feature

This document provides a series of detailed prompts for an AI coding agent to implement Feature #6 from the PRD: making audio files interactive and playable within LibreOffice documents. Follow these steps sequentially or in parallel as indicated in the guidelines at the end.

---
PROMPTS
---

1. [ ] Create the AudioPlayerControl UI Component

Human User Actions:
- None required for this step.

AI Prompt:
- Create a new custom VCL control named `AudioPlayerControl`.
- Create the necessary files: `vcl/source/control/audioplayer.cxx` and `vcl/inc/audioplayer.hxx`.
- The `AudioPlayerControl` class should inherit from `vcl::Control`.
- It should have member variables to hold the media URL and the filename.
- Implement the basic structure for painting the control. This initial version should draw a simple rectangle with a placeholder for a play/pause button and a text area for the filename. Do not implement full playback logic yet.
- It should use the `avmedia::Player` class from the `avmedia` module to handle audio playback. Create a placeholder instance of this player.
- Implement a `MouseButtonDown` event handler. For now, this handler can simply log to the console that it was clicked; the actual play/pause logic will be implemented in a later step.
- Ensure the new files are correctly added to the build system (e.g., the relevant `.mk` file in `vcl/`).

Unit Test:
- Create a basic test that instantiates an `AudioPlayerControl`.
- The test should verify that the control is created successfully and that its default state (e.g., filename string is empty, player is not active) is set correctly.

Human User Test:
- This component cannot be tested by a human user until it is integrated in a later step.

2. [ ] Reshape Audio Objects on Drop

Human User Actions:
- None required for this step.

AI Prompt:
- Modify the `SdrMediaObj` class located in `core/svx/source/svdraw/svdomedia.cxx`.
- When a media object is created from a dropped file, add logic to check the file's extension.
- If the file is a supported audio format (`.mp3`, `.wav`, `.m4a`), adjust the default geometry of the `SdrMediaObj` by calling `SetGeoRect` to create a wide rectangle (e.g., a 4:1 aspect ratio) instead of the default square.

Unit Test:
- It is difficult to unit test this directly. A manual test is more appropriate.

Human User Test:
- After this step is complete, build LibreOffice.
- Open the Writer application.
- Drag an MP3 file from your computer into the document.
- Verify that the media placeholder created in the document is a wide rectangle, not a square. It will likely still show the default musical note icon for now.

3. [ ] Integrate AudioPlayerControl into the Rendering Pipeline

Human User Actions:
- None required for this step. This step depends on the completion of step 1 and 2.

AI Prompt:
- This is a two-part task to replace the default media icon with our new control.
- Part A: Modify `ViewContactOfSdrMediaObj` in `core/svx/source/sdr/contact/viewcontactofsdrmediaobj.cxx`.
    - In the `createViewIndependentPrimitive2DSequence` method, extract the filename from the `SdrMediaObj`'s URL.
    - Pass this filename string down as a property to the `MediaPrimitive2D` that is created.
- Part B: Modify `MediaPrimitive2D` in `core/drawinglayer/source/primitive2d/mediaprimitive2d.cxx`.
    - In the `create2DDecomposition` method, add a check for the audio filename property passed from the step above.
    - If the filename property exists and corresponds to a supported audio file (`.mp3`, `.wav`, `.m4a`), the method should instantiate our new `AudioPlayerControl` instead of the default `GraphicPrimitive2D`.
    - The existing logic for drawing the object's background and border should be preserved.
    - Pass the media URL and filename to the `AudioPlayerControl` instance.

Unit Test:
- An integration-level test is most appropriate here. Manually verify the result.

Human User Test:
- Rebuild the LibreOffice application.
- Drag an MP3 file into Writer.
- Verify that you now see the custom `AudioPlayerControl` (a wide rectangle with a placeholder for the play button and the filename text). The old musical note icon should no longer be visible.

4. [ ] Implement Audio Playback Logic

Human User Actions:
- Have a sample MP3 file ready on your local machine for testing.

AI Prompt:
- Fully implement the playback functionality within the `AudioPlayerControl` in `vcl/source/control/audioplayer.cxx`.
- In the `MouseButtonDown` event handler, implement the play/pause logic.
- When the control is clicked:
    - If the audio is not playing, use the `avmedia::Player` instance to load the media from its URL and begin playback.
    - If the audio is already playing, call the `stop()` or `pause()` method on the player.
- Update the control's visual representation based on the playback state. For example, change the play button graphic to a pause graphic, and vice-versa.
- Ensure the filename passed to the control is correctly rendered below the play/pause button.

Unit Test:
- A unit test should be created that mocks the `avmedia::Player`.
- The test should trigger the `MouseButtonDown` event on the `AudioPlayerControl` and assert that the correct methods (`start()`, `stop()`, etc.) are called on the mocked player object based on its state.

Human User Test:
- Rebuild LibreOffice one last time.
- Open Writer and drag an audio file (e.g., an MP3) into the document.
- Verify that the control displays the correct filename.
- Click the play button. The audio should begin playing.
- Click the button again. The audio should stop or pause.
- Verify that the button's appearance changes to reflect the current playback state.

---
## Parallel Implementation Guidelines

To accelerate the development of this feature, the prompts above can be executed by multiple AI agents working in parallel. The following workflow outlines the dependencies and opportunities for parallel work.

1.  **Start in Parallel:** Prompts **1** and **2** can be started at the same time.
    -   Agent A works on **Prompt 1 (`Create the AudioPlayerControl UI Component`)**.
    -   Agent B works on **Prompt 2 (`Reshape Audio Objects on Drop`)**.
    -   These tasks are independent, affecting different modules (`vcl` vs. `svx`).

2.  **Continue Dependent Task:** Once Agent A completes **Prompt 1**, a new agent (or Agent A) can immediately begin **Prompt 4 (`Implement Audio Playback Logic`)**. This work is contained within the new `AudioPlayerControl` and does not depend on other steps.

3.  **Final Integration:** Once Agent B has completed **Prompt 2** and the agent working on **Prompt 4** has finished, a final agent can execute **Prompt 3 (`Integrate AudioPlayerControl into the Rendering Pipeline`)**. This step is the integration point and depends on the completion of the prior tasks: the control must exist and be fully functional, and the rendering pipeline must be ready to accept it.

This approach allows for the core component logic, the object model modifications, and the final integration to be worked on concurrently, reducing the total time to feature completion. 