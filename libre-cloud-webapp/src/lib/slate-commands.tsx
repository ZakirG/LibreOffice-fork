import React from 'react';
import { Editor, Element, Transforms, Text } from 'slate';

// Text formatting marks
export type FormatType = 'bold' | 'italic' | 'code';

// Block types
export type BlockType = 'paragraph' | 'heading' | 'code-block';

/**
 * Check if a mark is active at the current selection
 */
export const isMarkActive = (editor: Editor, format: FormatType): boolean => {
  const marks = Editor.marks(editor) as any;
  return marks ? marks[format] === true : false;
};

/**
 * Check if a block type is active at the current selection
 */
export const isBlockActive = (editor: Editor, blockType: BlockType): boolean => {
  const { selection } = editor;
  if (!selection) return false;

  const [match] = Array.from(
    Editor.nodes(editor, {
      at: Editor.unhangRange(editor, selection),
      match: n =>
        !Editor.isEditor(n) && Element.isElement(n) && (n as any).type === blockType,
    })
  );

  return !!match;
};

/**
 * Toggle a text formatting mark
 */
export const toggleMark = (editor: Editor, format: FormatType): void => {
  const isActive = isMarkActive(editor, format);

  if (isActive) {
    Editor.removeMark(editor, format);
  } else {
    Editor.addMark(editor, format, true);
  }
};

/**
 * Toggle a block type
 */
export const toggleBlock = (editor: Editor, blockType: BlockType): void => {
  const isActive = isBlockActive(editor, blockType);
  
  Transforms.setNodes(
    editor,
    { type: isActive ? 'paragraph' : blockType } as Partial<Element>,
    { match: n => !Editor.isEditor(n) && Element.isElement(n) }
  );
};

/**
 * Handle keyboard shortcuts
 */
export const handleKeyDown = (editor: Editor, event: React.KeyboardEvent): boolean => {
  const { key, ctrlKey, metaKey } = event;
  const isModKey = ctrlKey || metaKey;

  if (!isModKey) return false;

  switch (key) {
    case 'b':
      event.preventDefault();
      toggleMark(editor, 'bold');
      return true;
    case 'i':
      event.preventDefault();
      toggleMark(editor, 'italic');
      return true;
    case '`':
      event.preventDefault();
      toggleMark(editor, 'code');
      return true;
    default:
      return false;
  }
};

/**
 * Render leaf elements (text with formatting)
 */
export const renderLeaf = ({ attributes, children, leaf }: any) => {
  if (leaf.bold) {
    children = <strong>{children}</strong>;
  }

  if (leaf.italic) {
    children = <em>{children}</em>;
  }

  if (leaf.code) {
    children = (
      <code className="bg-gray-100 px-1 py-0.5 rounded text-sm font-mono">
        {children}
      </code>
    );
  }

  return <span {...attributes}>{children}</span>;
};

/**
 * Render block elements
 */
export const renderElement = ({ attributes, children, element }: any) => {
  switch (element.type) {
    case 'heading':
      return (
        <h2 {...attributes} className="text-xl font-bold mb-2">
          {children}
        </h2>
      );
    case 'code-block':
      return (
        <pre
          {...attributes}
          className="bg-gray-100 p-3 rounded font-mono text-sm overflow-x-auto"
        >
          <code>{children}</code>
        </pre>
      );
    default:
      return (
        <p {...attributes} className="mb-2">
          {children}
        </p>
      );
  }
}; 