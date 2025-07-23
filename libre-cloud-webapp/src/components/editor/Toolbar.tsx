'use client';

import { Editor } from 'slate';
import { toggleMark, isMarkActive, FormatType } from '@/lib/slate-commands';

interface ToolbarProps {
  editor?: Editor;
  onSave?: () => void;
  isSaving?: boolean;
  isDirty?: boolean;
  saveStatus?: 'idle' | 'saving' | 'saved' | 'error';
}

interface FormatButtonProps {
  format: FormatType;
  icon: React.ReactNode;
  title: string;
  editor?: Editor;
}

function FormatButton({ format, icon, title, editor }: FormatButtonProps) {
  const isActive = editor ? isMarkActive(editor, format) : false;
  
  const handleClick = () => {
    if (editor) {
      toggleMark(editor, format);
    }
  };

  return (
    <button
      type="button"
      onClick={handleClick}
      title={title}
      className={`px-3 py-1 text-sm border border-gray-300 rounded hover:bg-gray-50 text-gray-900 font-medium ${
        isActive ? 'bg-blue-100 border-blue-400 text-blue-800' : ''
      }`}
    >
      {icon}
    </button>
  );
}

export default function Toolbar({ editor, onSave, isSaving = false, isDirty = false, saveStatus = 'idle' }: ToolbarProps) {
  return (
    <div className="border border-gray-200 rounded-lg bg-white p-3 mb-4">
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-2">
          <FormatButton 
            format="bold" 
            icon={<strong>B</strong>} 
            title="Bold (Ctrl+B)"
            editor={editor}
          />
          <FormatButton 
            format="italic" 
            icon={<em>I</em>} 
            title="Italic (Ctrl+I)"
            editor={editor}
          />
          <FormatButton 
            format="code" 
            icon="Code" 
            title="Code (Ctrl+`)"
            editor={editor}
          />
        </div>
        
        <div className="flex items-center space-x-2">
          {saveStatus === 'saved' && !isDirty && (
            <span className="text-sm text-green-600 flex items-center">
              <svg className="w-4 h-4 mr-1" fill="currentColor" viewBox="0 0 20 20">
                <path fillRule="evenodd" d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z" clipRule="evenodd" />
              </svg>
              Saved
            </span>
          )}
          {isDirty && saveStatus !== 'saving' && (
            <span className="text-sm text-gray-500">Unsaved changes</span>
          )}
          <button
            type="button"
            onClick={onSave}
            disabled={!isDirty || isSaving}
            className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            {isSaving ? 'Saving...' : 'Save'}
          </button>
        </div>
      </div>
    </div>
  );
} 