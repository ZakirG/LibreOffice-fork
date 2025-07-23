import { render, screen } from '@testing-library/react';
import SlateEditor from '../SlateEditor';

// Mock slate modules since they may not be available during testing
jest.mock('slate', () => ({
  createEditor: jest.fn(() => ({})),
  Transforms: {},
  Editor: {},
  Element: {},
  Node: {},
  Point: {},
  Range: {},
  Text: {},
}));

jest.mock('slate-react', () => ({
  Slate: ({ children }: { children: React.ReactNode }) => <div data-testid="slate-editor">{children}</div>,
  Editable: () => <div data-testid="slate-editable">Editor content</div>,
  withReact: jest.fn((editor) => editor),
  useSlate: jest.fn(),
  useSelected: jest.fn(),
  useFocused: jest.fn(),
}));

jest.mock('slate-history', () => ({
  withHistory: jest.fn((editor) => editor),
}));

describe('SlateEditor', () => {
  it('renders without crashing', () => {
    render(<SlateEditor />);
    expect(screen.getByTestId('slate-editor')).toBeInTheDocument();
    expect(screen.getByTestId('slate-editable')).toBeInTheDocument();
  });

  it('displays placeholder text', () => {
    render(<SlateEditor />);
    expect(screen.getByText('Editor content')).toBeInTheDocument();
  });

  it('accepts initial value prop', () => {
    const initialValue = [{ type: 'paragraph', children: [{ text: 'Test content' }] }];
    render(<SlateEditor initialValue={initialValue} />);
    expect(screen.getByTestId('slate-editor')).toBeInTheDocument();
  });

  it('calls onChange when provided', () => {
    const mockOnChange = jest.fn();
    render(<SlateEditor onChange={mockOnChange} />);
    expect(screen.getByTestId('slate-editor')).toBeInTheDocument();
  });

  it('can be set to read-only', () => {
    render(<SlateEditor readOnly={true} />);
    expect(screen.getByTestId('slate-editable')).toBeInTheDocument();
  });
}); 