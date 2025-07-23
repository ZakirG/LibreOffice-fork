import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import DocumentEditor from '../DocumentEditor';

// Mock the child components and slate modules
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

// Mock fetch
global.fetch = jest.fn();

const mockFetch = fetch as jest.MockedFunction<typeof fetch>;

describe('DocumentEditor', () => {
  beforeEach(() => {
    mockFetch.mockClear();
  });

  it('renders loading state initially', () => {
    mockFetch.mockImplementation(() => 
      new Promise(() => {}) // Never resolves to keep loading state
    );

    render(<DocumentEditor documentId="test-doc" />);
    expect(screen.getByText('Loading document...')).toBeInTheDocument();
  });

  it('renders editor when document loads successfully', async () => {
    mockFetch
      .mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ presignedUrl: 'https://example.com/doc' }),
      } as Response)
      .mockResolvedValueOnce({
        ok: true,
        text: () => Promise.resolve('[{"type":"paragraph","children":[{"text":"Hello World!"}]}]'),
      } as Response);

    render(<DocumentEditor documentId="test-doc" />);

    await waitFor(() => {
      expect(screen.getByTestId('slate-editor')).toBeInTheDocument();
    });
  });

  it('handles new document (404) gracefully', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: false,
      status: 404,
    } as Response);

    render(<DocumentEditor documentId="new-doc" />);

    await waitFor(() => {
      expect(screen.getByTestId('slate-editor')).toBeInTheDocument();
    });
  });

  it('handles empty document content', async () => {
    mockFetch
      .mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ presignedUrl: 'https://example.com/doc' }),
      } as Response)
      .mockResolvedValueOnce({
        ok: true,
        text: () => Promise.resolve(''),
      } as Response);

    render(<DocumentEditor documentId="empty-doc" />);

    await waitFor(() => {
      expect(screen.getByTestId('slate-editor')).toBeInTheDocument();
    });
  });

  it('shows error for invalid JSON content', async () => {
    mockFetch
      .mockResolvedValueOnce({
        ok: true,
        json: () => Promise.resolve({ presignedUrl: 'https://example.com/doc' }),
      } as Response)
      .mockResolvedValueOnce({
        ok: true,
        text: () => Promise.resolve('invalid json'),
      } as Response);

    render(<DocumentEditor documentId="invalid-doc" />);

    await waitFor(() => {
      expect(screen.getByText(/Document is not in a supported format/)).toBeInTheDocument();
    });
  });

  it('shows error for fetch failure', async () => {
    mockFetch.mockRejectedValueOnce(new Error('Network error'));

    render(<DocumentEditor documentId="error-doc" />);

    await waitFor(() => {
      expect(screen.getByText(/Network error/)).toBeInTheDocument();
      expect(screen.getByText('Try Again')).toBeInTheDocument();
    });
  });

  it('allows retry after error', async () => {
    mockFetch
      .mockRejectedValueOnce(new Error('Network error'))
      .mockResolvedValueOnce({
        ok: false,
        status: 404,
      } as Response);

    render(<DocumentEditor documentId="retry-doc" />);

    await waitFor(() => {
      expect(screen.getByText('Try Again')).toBeInTheDocument();
    });

    fireEvent.click(screen.getByText('Try Again'));

    await waitFor(() => {
      expect(screen.getByTestId('slate-editor')).toBeInTheDocument();
    });
  });

  it('handles presigned URL failure', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: false,
      status: 500,
    } as Response);

    render(<DocumentEditor documentId="presign-error-doc" />);

    await waitFor(() => {
      expect(screen.getByText(/Failed to get presigned URL: 500/)).toBeInTheDocument();
    });
  });
}); 