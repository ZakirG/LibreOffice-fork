# Implementation Plan: In-App Document Editor with Slate.js

This document outlines the implementation plan for integrating a rich text editor into the web application using Slate.js. This will allow users to create, edit, and save documents directly in their browser.

**Scope and Limitations:**
- This initial implementation will focus on editing documents stored in Slate's native JSON format.
- It will not support editing existing binary formats like `.odt` or `.docx`. The editor will be used for creating new "rich text" documents.
- The dashboard will be updated to differentiate between these new rich text documents and other uploaded files.

---

### PHASE 1: Project Setup and Editor Scaffolding

**Goal:** Install necessary dependencies and create the basic file structure for the editor page and its components.

**Human Actions Required:**
- None.

**AI Prompt:**
1.  **Install Dependencies:** Add `slate`, `slate-react`, and `slate-history` to the `libre-cloud-webapp` project's `package.json`.
2.  **Modify Document Page:** Update `/app/doc/[id]/page.tsx` to serve as the main container for the editor. This page will be responsible for fetching data and managing the editor's state.
3.  **Create Editor Component:** Create a new client component at `/app/components/editor/SlateEditor.tsx`. This will encapsulate the core Slate editor logic.
4.  **Create Toolbar Component:** Create a new component at `/app/components/editor/Toolbar.tsx`. This will hold formatting buttons.
5.  **Initial UI:** The `SlateEditor.tsx` and `Toolbar.tsx` should render with a basic, non-functional UI for now.

**Unit Test:**
- Verify `slate`, `slate-react`, and `slate-history` are in `package.json`.
- Write a basic render test for `SlateEditor.tsx` and `Toolbar.tsx`.

**Human Test:**
- Run the web app.
- Navigate to `/doc/some-fake-id`.
- Verify that a page loads containing a placeholder for the editor and toolbar without errors.

---

### PHASE 2: Fetching and Rendering Document Content

**Goal:** Load a document's content from S3 and render it in the Slate editor.

**Human Actions Required:**
- Manually upload a test file to S3 containing valid Slate JSON to be used for testing. The file content should look something like this: `[{"type":"paragraph","children":[{"text":"Hello World!"}]}]`. Name it something like `test-doc.slate`.

**AI Prompt:**
1.  **Data Fetching Logic:** In `/app/doc/[id]/page.tsx`, implement the following logic:
    - On page load, get the document ID from the URL params.
    - Call the `/api/presign` endpoint (with `mode: 'GET'`) to obtain a presigned URL for the document from S3.
    - Fetch the document content from the presigned URL.
2.  **Content Handling:**
    - The fetched content is expected to be a JSON string. Parse it into a JavaScript object.
    - If the content is empty or the fetch fails (e.g., a new document), initialize the editor with a default empty value: `[{ type: 'paragraph', children: [{ text: '' }] }]`.
    - If parsing fails, display an error message indicating the document is not a supported format.
3.  **Pass to Editor:** Pass the parsed Slate document object as a prop to the `SlateEditor.tsx` component.
4.  **Editor Rendering:** In `SlateEditor.tsx`, use the received prop to initialize the Slate editor's state.
5.  **UI States:** Implement loading and error states in the UI (e.g., show a loading spinner while fetching, and an error message on failure).

**Unit Test:**
- Test the data fetching logic in `/app/doc/[id]/page.tsx` by mocking the API calls.
- Test the content parsing and fallback to a default value.
- Test that `SlateEditor.tsx` correctly renders the initial value.

**Human Test:**
- Navigate to the URL for the manually uploaded test document.
- Verify the content "Hello World!" is displayed in the editor.
- Navigate to a URL for a new document ID and verify an empty editor is shown.
- Test the loading and error states by simulating network conditions or pointing to an invalid file.

---

### PHASE 3: Saving Document Content

**Goal:** Enable users to save their changes back to S3.

**Human Actions Required:**
- None.

**AI Prompt:**
1.  **Add Save Button:** Add a "Save" button to the UI, for instance in the `Toolbar.tsx`.
2.  **Track Changes:** Implement logic to track if the document has been modified (`isDirty` state). The "Save" button should be disabled if no changes have been made.
3.  **Saving Logic:** On clicking "Save":
    - Get the current value from the Slate editor state.
    - Convert the Slate JSON object to a string using `JSON.stringify`.
    - Request a presigned `PUT` URL from the `/api/presign` endpoint. The request should specify `Content-Type: application/json`.
    - Use the presigned URL to upload the JSON string to S3.
    - After a successful upload, make a `PATCH` request to a new `/api/documents/[id]` endpoint to update the `lastModified` timestamp in DynamoDB.
4.  **API Endpoint:** Create the new API route `/app/api/documents/[id]/route.ts` that handles `PATCH` requests to update document metadata.
5.  **User Feedback:** Implement UI feedback for the save process (e.g., "Saving...", "Saved!", "Error saving document").

**Unit Test:**
- Test the `isDirty` state logic.
- Test the save function, mocking API calls for presigned URL, S3 upload, and metadata update.
- Write tests for the new `PATCH /api/documents/[id]` endpoint.

**Human Test:**
- Open a document, make a change, and see the "Save" button become active.
- Click "Save" and verify the UI feedback.
- Refresh the page and confirm the changes are persisted.
- Check the `lastModified` date on the main dashboard to see that it has updated.
- Inspect the raw file in S3 to confirm it contains the updated JSON.

---

### PHASE 4: Toolbar Implementation and Basic Formatting

**Goal:** Implement a functional toolbar with basic formatting options like bold, italic, and code blocks.

**Human Actions Required:**
- None.

**AI Prompt:**
1.  **Toolbar Logic:** In `Toolbar.tsx`, implement the `onClick` handlers for the formatting buttons.
2.  **Editor Commands:** Create helper functions within the editor component (`SlateEditor.tsx` or a separate `lib/slate-commands.ts`) to apply and remove formatting marks (e.g., bold, italic) from the selected text.
3.  **Keyboard Shortcuts:** Add an `onKeyDown` handler to the Slate editor to support common keyboard shortcuts (e.g., `Cmd/Ctrl+B` for bold).
4.  **Active State:** The toolbar buttons should dynamically update their appearance to reflect the formatting of the text at the current cursor position or selection. For example, if the cursor is in a bolded word, the "Bold" button should appear active.

**Unit Test:**
- Test the editor command functions for applying/removing marks.
- Test that keyboard shortcuts trigger the correct commands.
- Test that the toolbar button's active state is correctly synchronized with the editor's selection.

**Human Test:**
- Open the editor and type some text.
- Use the toolbar buttons to apply bold, italic, and other formatting.
- Use keyboard shortcuts to do the same.
- Verify that formatting is applied correctly and that the toolbar buttons reflect the current style.
- Save the document, reload, and verify that the formatting is preserved. 