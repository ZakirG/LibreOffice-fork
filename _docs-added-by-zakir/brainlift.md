# My PRD

[TODO: place PRD here once done]


# Studying the Codebase and Building Targeted Documentation for New Features

Me to Gemini 2.5 Pro:
Deep dive through the codebase and only look at the readme files in each folder. Do not look at the actual code, only look at the readme files in each folder. And then come back with what you learned and add it to my high-level overview document in an easy-understand way so that I can understand this codebase before I add new features to it. I am a software engineer.

The output is in the file codebase_overview.md.


Me to Gemini 2.5 Pro:
Please write a markdown document discussing the tech stack of this codebase. 

The output was added to codebase_overview.md.


Me:
We will be modifying the current codebase according to this new PRD: @PRD.md  Please deep dive the codebase and consider which parts of the codebase we will need to change and which parts of the codebase are important to understand in order to implement these new features. But in your deep dive, only focus on the readme files. Do not look at the actual code because the actual code is way too big. So just focus on the readme files. And everything that you learn, put it in a new document in our docs folder _docs-added-by-zakir. And put that new markdown file in that folder and make your explanation as technical as possible. Name your file codebase_details_related_to_new_features.md. In your explanation, reference the folder directories by name and any other technical terms as they relate to our codebase. And any conventions that the codebase has set that we will need to follow in order to respect the existing conventions and maintain existing functionalities without causing regressions. Lets do it! 

[output not shown]

Me:
very good. i would like you to revise this document you just made, adding new details. for each segment of code you called out in your first version, i want you to read that specific code now, and add supporting details in each section that might assist with our understanding and the implementation plan.

[output not shown]

Me:
looks good. lets make another revision -- i want you to use the codebase to add even more detail for an external user. what is UNO, how does it look and how does it work? what are some design considerations when following the UNO model? what are some important interfaces we need to follow when interacting with libraries and frameworks mentioned in our document?

[output not shown]


# Exploratory conversations

Me to ChatGPT 4o:
what is LibreOffice

ChatGPT 4o:
LibreOffice is a free and open-source office suite developed by The Document Foundation. It’s similar to Microsoft Office and includes several powerful applications for word processing, spreadsheets, presentations, and more.

Here's a quick breakdown of the main components:

Writer – Word processor (like Microsoft Word)

Calc – Spreadsheet application (like Excel)

Impress – Presentation tool (like PowerPoint)

Draw – Vector graphics and diagramming tool (like Visio)

Base – Database management system (like Access)

Math – Formula and equation editor

LibreOffice runs on Windows, macOS, and Linux. It uses the Open Document Format (ODF) by default but can also open and save Microsoft Office file formats (.docx, .xlsx, .pptx, etc.).

Because it’s open-source, anyone can modify the source code. This makes it a great choice for custom projects—like your idea of tailoring it to a specific niche.








