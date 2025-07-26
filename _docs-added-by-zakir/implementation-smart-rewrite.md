Here is a series of prompts to implement the "Smart Rewrite with AI" feature.

---

1. [ ] API Key and Configuration Setup

Human User Actions:
- Obtain an API key from a commercial AI text-generation service (e.g., OpenAI, Google AI, Anthropic).
- Identify the API endpoint URL for the service.

AI Prompt:
- Modify the `officecfg` component to securely store and retrieve the external AI service API key and endpoint URL.
- Create a new configuration schema for these settings. Do not hardcode the key in the source code.
- Implement a C++ helper function, possibly in a new `SmartRewriteService` class within the `sw` module, to read these configuration values. The function should be easily accessible to the feature's main handler.
- For now, if the configuration is not found, the feature should be gracefully disabled (e.g., the context menu item does not appear).

Unit Test:
- A unit test for this would involve creating a temporary configuration file with a dummy API key, running the C++ helper function to read it, and asserting that the correct key is returned. Given the complexity of setting up a test harness for LibreOffice components, this may be deferred in favor of the manual user test.

Human User Test:
- The user will need to find the user profile configuration file for LibreOffice (e.g., `user/registrymodifications.xcu`).
- Manually add the XML entries for the new API key and endpoint URL configuration.
- Run a debug build of LibreOffice. When the application starts, check the debug logs to confirm that the new helper function correctly reads and logs the API key and URL from the configuration file.

---

2. [ ] Create the Smart Rewrite UI Dialog

Human User Actions:
- None.

AI Prompt:
- Create a new dialog UI definition file at `sw/uiconfig/scw/ui/smartrewritedialog.ui`.
- Following the pattern in `sw/source/ui/fldui/DropDownFieldDialog.cxx`, define a dialog layout using XML.
- The dialog must contain:
  - A `ComboBox` for pre-defined rewrite styles (e.g., "Professional", "Concise", "Friendly").
  - A multi-line `Edit` field for the user to enter a custom rewrite prompt.
  - An "OK" button and a "Cancel" button.
- Create the C++ controller class skeleton `SmartRewriteDialogController` in `sw/source/ui/smartrewrite/` (new directory) that is responsible for loading and managing this dialog. It should use the `vcl` and `weld` libraries. At this stage, the dialog only needs to be defined; it does not need to be hooked up to any logic.

Unit Test:
- A unit test is not practical for just the UI definition. The functionality will be verified in a later step.

Human User Test:
- This functionality cannot be tested directly by the user until it is triggered by the context menu in a later step.

---

3. [ ] Implement and Register the Context Menu Interceptor

Human User Actions:
- None.

AI Prompt:
- In the `sw` module (e.g., in `sw/source/uibase/uno/`), create a new class `SmartRewriteInterceptor` that implements the `com.sun.star.frame.XDispatchProviderInterceptor` interface.
- Follow the implementation pattern of `SwXDispatchProviderInterceptor` found in `sw/source/uibase/uno/unodispatch.cxx`.
- The interceptor should watch for the context menu dispatch. If the context menu is triggered over a text selection, it must dynamically add a new menu item labeled "Smart Rewrite with AI".
- Associate this new menu item with a new custom UNO command: `.uno:SmartRewrite`.
- Register this new interceptor with the `SwView`'s frame so that it becomes active.

Unit Test:
- A unit test would involve programmatically creating a view, dispatching a context menu event over a selection, and verifying that the interceptor correctly modifies the menu's contents. This is complex to set up.

Human User Test:
- Launch LibreOffice Writer.
- Open a document and select a piece of text.
- Right-click on the selected text.
- Verify that a new option "Smart Rewrite with AI" appears in the context menu. Clicking it will have no effect yet.

---

4. [ ] Connect the Context Menu to the Dialog

Human User Actions:
- None.

AI Prompt:
- Create a new handler class, `SmartRewriteHandler`, in the `sw` module.
- Implement the necessary dispatch logic to map the `.uno:SmartRewrite` command to this handler.
- When the handler is invoked, it should instantiate and display the `SmartRewriteDialogController` and its associated dialog (from step 2) as a modal window.
- The handler should first get the currently selected text from the document view (`SwPaM`). This text is not yet used but should be available to the handler.

Unit Test:
- Not applicable; this is integration logic best tested manually.

Human User Test:
- Launch LibreOffice Writer.
- Select a piece of text and right-click.
- Click the "Smart Rewrite with AI" menu item.
- Verify that the dialog created in step 2 now appears on the screen. Clicking "OK" or "Cancel" should simply close the dialog.

---

5. [ ] Implement Asynchronous API Call

Human User Actions:
- Ensure the API key and endpoint are correctly configured from step 1.
- If using a live API, ensure your account has credits.

AI Prompt:
- Enhance the `SmartRewriteHandler`. When the user clicks "OK" in the dialog:
  - Retrieve the selected text, the rewrite style from the `ComboBox`, and the custom prompt from the `Edit` field.
  - Construct the appropriate JSON payload for the external AI service.
  - Using the `CurlSession` infrastructure (or a similar existing HTTP client), make a `POST` request to the AI service endpoint.
  - This network request MUST be performed asynchronously on a background thread to prevent freezing the application UI. Use the `SolarMutex` correctly for any UI updates.
  - For now, upon receiving the response, log the full JSON output to the debug console. Implement basic error handling for network failures or non-200 status codes, showing an error message to the user via a `vcl` dialog.

Unit Test:
- This could be tested by creating a mock `CurlSession` that returns a predefined JSON response, allowing verification of the JSON parsing and handler logic without a live network call.

Human User Test:
- Select a piece of text in Writer and invoke the feature.
- In the dialog, select a style and click "OK".
- The UI should remain responsive.
- Check the application's debug console output. Verify that it contains the expected JSON response from the AI service.

---

6. [ ] Implement Text Replacement and Undo Functionality

Human User Actions:
- None.

AI Prompt:
- Modify the `SmartRewriteHandler`'s asynchronous callback logic.
- Upon receiving a successful API response, ensure the code dispatches execution back to the main UI thread.
- Before modifying the document, create a custom undo object that stores the original selected text and selection range.
- Add this object to the document's undo manager using `GetIDocumentUndoRedo()->AddUndo(...)`. This makes the entire operation atomic and reversible.
- Parse the rewritten text from the API's JSON response.
- Use the `IDocumentContentOperations` interface to replace the current text selection with the new, rewritten text.
- Close the Smart Rewrite dialog.

Unit Test:
- A test would involve creating a document in memory, setting a selection with known text, running the handler's replacement logic with mock API data, and then programmatically invoking undo/redo to verify the document state changes correctly.

Human User Test:
- Select a piece of text and use the "Smart Rewrite with AI" feature.
- Verify that the selected text is replaced by the AI-generated text.
- Press Ctrl+Z (or select Edit -> Undo).
- Verify that the text reverts to its original state.
- Press Ctrl+Y (or select Edit -> Redo).
- Verify that the text changes back to the AI-generated version.

---

## Parallel Implementation Guidelines

The implementation of this feature can be partially parallelized to accelerate development.

1.  **Phase 1 (Parallel Tasks):** Prompts 1, 2, and 3 can be worked on simultaneously as they are largely independent.
    -   **Task A:** Developer 1 works on **Prompt 1 [API Key and Configuration Setup]**. This is a foundational task that deals with external configuration.
    -   **Task B:** Developer 2 works on **Prompt 2 [Create the Smart Rewrite UI Dialog]**. This focuses purely on the UI definition and its C++ controller shell.
    -   **Task C:** Developer 3 works on **Prompt 3 [Implement and Register the Context Menu Interceptor]**. This task is about hooking into the application's UI command system.

2.  **Phase 2 (Integration):** Once Prompts 2 and 3 are complete, work can begin on Prompt 4.
    -   **Task D:** A developer can now take on **Prompt 4 [Connect the Context Menu to the Dialog]**, which integrates the work from Task B and Task C.

3.  **Phase 3 (Core Logic):** Once Prompts 1 and 4 are complete, the main feature logic can be implemented.
    -   **Task E:** A developer can begin **Prompt 5 [Implement Asynchronous API Call]**. This requires the dialog to be present (Task D) and the API key to be accessible (Task A).

4.  **Phase 4 (Finalization):** Once Prompt 5 is functional, the final step can be completed.
    -   **Task F:** A developer can complete **Prompt 6 [Implement Text Replacement and Undo Functionality]**, building directly on the successful API communication from Task E.