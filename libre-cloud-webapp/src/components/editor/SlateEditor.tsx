'use client';

import { useState, useMemo, useEffect, useCallback } from 'react';
import { createEditor, Descendant } from 'slate';
import { Slate, Editable, withReact } from 'slate-react';
import { withHistory } from 'slate-history';
import { handleKeyDown, renderLeaf, renderElement } from '@/lib/slate-commands';

interface SlateEditorProps {
  initialValue?: Descendant[];
  onChange?: (value: Descendant[]) => void;
  readOnly?: boolean;
  onEditorReady?: (editor: any) => void;
}

interface ParagraphElement {
  type: 'paragraph';
  children: Descendant[];
}

const defaultValue: Descendant[] = [
  {
    type: 'paragraph',
    children: [{ text: '' }],
  } as ParagraphElement,
];

export default function SlateEditor({ 
  initialValue = defaultValue, 
  onChange,
  readOnly = false,
  onEditorReady
}: SlateEditorProps) {
  const [value, setValue] = useState<Descendant[]>(initialValue);
  
  const editor = useMemo(() => withHistory(withReact(createEditor())), []);

  // Update local state when initialValue changes
  useEffect(() => {
    setValue(initialValue);
  }, [initialValue]);

  // Notify parent when editor is ready
  useEffect(() => {
    if (onEditorReady) {
      onEditorReady(editor);
    }
  }, [editor, onEditorReady]);

  const handleChange = (newValue: Descendant[]) => {
    setValue(newValue);
    onChange?.(newValue);
  };

  const onKeyDown = useCallback((event: React.KeyboardEvent) => {
    handleKeyDown(editor, event);
  }, [editor]);

  return (
    <div className="border border-gray-200 rounded-lg min-h-[400px] p-4 bg-white">
      <Slate
        editor={editor}
        initialValue={value}
        onValueChange={handleChange}
      >
        <Editable
          readOnly={readOnly}
          placeholder="Start typing your document..."
          className="outline-none min-h-[360px] text-gray-900"
          onKeyDown={onKeyDown}
          renderLeaf={renderLeaf}
          renderElement={renderElement}
        />
      </Slate>
    </div>
  );
} 