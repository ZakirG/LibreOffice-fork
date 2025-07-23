import { createEditor } from 'slate';
import { withReact } from 'slate-react';
import { withHistory } from 'slate-history';
import { 
  isMarkActive, 
  toggleMark, 
  handleKeyDown,
  renderLeaf,
  renderElement 
} from '../slate-commands';

// Mock slate modules
jest.mock('slate', () => ({
  createEditor: jest.fn(() => ({
    marks: {},
    selection: null,
  })),
  Editor: {
    marks: jest.fn(),
    addMark: jest.fn(),
    removeMark: jest.fn(),
    isEditor: jest.fn(),
    nodes: jest.fn(() => []),
    unhangRange: jest.fn(),
  },
  Element: {
    isElement: jest.fn(),
  },
  Transforms: {
    setNodes: jest.fn(),
  },
}));

jest.mock('slate-react', () => ({
  withReact: jest.fn((editor) => editor),
}));

jest.mock('slate-history', () => ({
  withHistory: jest.fn((editor) => editor),
}));

const mockEditor = {
  marks: {},
  selection: null,
  addMark: jest.fn(),
  removeMark: jest.fn(),
} as any;

const mockMarks = require('slate').Editor.marks as jest.MockedFunction<any>;
const mockAddMark = require('slate').Editor.addMark as jest.MockedFunction<any>;
const mockRemoveMark = require('slate').Editor.removeMark as jest.MockedFunction<any>;

describe('slate-commands', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  describe('isMarkActive', () => {
    it('returns true when mark is active', () => {
      mockMarks.mockReturnValueOnce({ bold: true });
      expect(isMarkActive(mockEditor, 'bold')).toBe(true);
    });

    it('returns false when mark is not active', () => {
      mockMarks.mockReturnValueOnce({ italic: true });
      expect(isMarkActive(mockEditor, 'bold')).toBe(false);
    });

    it('returns false when no marks exist', () => {
      mockMarks.mockReturnValueOnce(null);
      expect(isMarkActive(mockEditor, 'bold')).toBe(false);
    });
  });

  describe('toggleMark', () => {
    it('removes mark when active', () => {
      mockMarks.mockReturnValueOnce({ bold: true });
      toggleMark(mockEditor, 'bold');
      expect(mockRemoveMark).toHaveBeenCalledWith(mockEditor, 'bold');
    });

    it('adds mark when not active', () => {
      mockMarks.mockReturnValueOnce({});
      toggleMark(mockEditor, 'bold');
      expect(mockAddMark).toHaveBeenCalledWith(mockEditor, 'bold', true);
    });
  });

  describe('handleKeyDown', () => {
    const createKeyEvent = (key: string, ctrlKey = true): React.KeyboardEvent => ({
      key,
      ctrlKey,
      metaKey: false,
      preventDefault: jest.fn(),
    } as any);

    it('handles bold shortcut (Ctrl+B)', () => {
      const event = createKeyEvent('b');
      mockMarks.mockReturnValueOnce({});
      
      const result = handleKeyDown(mockEditor, event);
      
      expect(result).toBe(true);
      expect(event.preventDefault).toHaveBeenCalled();
      expect(mockAddMark).toHaveBeenCalledWith(mockEditor, 'bold', true);
    });

    it('handles italic shortcut (Ctrl+I)', () => {
      const event = createKeyEvent('i');
      mockMarks.mockReturnValueOnce({});
      
      const result = handleKeyDown(mockEditor, event);
      
      expect(result).toBe(true);
      expect(event.preventDefault).toHaveBeenCalled();
      expect(mockAddMark).toHaveBeenCalledWith(mockEditor, 'italic', true);
    });

    it('handles code shortcut (Ctrl+`)', () => {
      const event = createKeyEvent('`');
      mockMarks.mockReturnValueOnce({});
      
      const result = handleKeyDown(mockEditor, event);
      
      expect(result).toBe(true);
      expect(event.preventDefault).toHaveBeenCalled();
      expect(mockAddMark).toHaveBeenCalledWith(mockEditor, 'code', true);
    });

    it('returns false for non-shortcut keys', () => {
      const event = createKeyEvent('a');
      
      const result = handleKeyDown(mockEditor, event);
      
      expect(result).toBe(false);
      expect(event.preventDefault).not.toHaveBeenCalled();
    });

    it('returns false when no modifier key is pressed', () => {
      const event = createKeyEvent('b', false);
      
      const result = handleKeyDown(mockEditor, event);
      
      expect(result).toBe(false);
    });
  });

  describe('renderLeaf', () => {
    const mockProps = {
      attributes: { 'data-slate-leaf': true },
      children: 'test text',
      leaf: {},
    };

    it('renders plain text', () => {
      const result = renderLeaf(mockProps);
      expect(result.type).toBe('span');
      expect(result.props.children).toBe('test text');
    });

    it('renders bold text', () => {
      const props = { ...mockProps, leaf: { bold: true } };
      const result = renderLeaf(props);
      expect(result.props.children.type).toBe('strong');
    });

    it('renders italic text', () => {
      const props = { ...mockProps, leaf: { italic: true } };
      const result = renderLeaf(props);
      expect(result.props.children.type).toBe('em');
    });

    it('renders code text', () => {
      const props = { ...mockProps, leaf: { code: true } };
      const result = renderLeaf(props);
      expect(result.props.children.type).toBe('code');
    });
  });

  describe('renderElement', () => {
    const mockProps = {
      attributes: { 'data-slate-node': 'element' },
      children: 'test content',
      element: { type: 'paragraph' },
    };

    it('renders paragraph by default', () => {
      const result = renderElement(mockProps);
      expect(result.type).toBe('p');
      expect(result.props.children).toBe('test content');
    });

    it('renders heading', () => {
      const props = { ...mockProps, element: { type: 'heading' } };
      const result = renderElement(props);
      expect(result.type).toBe('h2');
    });

    it('renders code block', () => {
      const props = { ...mockProps, element: { type: 'code-block' } };
      const result = renderElement(props);
      expect(result.type).toBe('pre');
    });
  });
}); 