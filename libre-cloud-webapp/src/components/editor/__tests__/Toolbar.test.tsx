import { render, screen, fireEvent } from '@testing-library/react';
import Toolbar from '../Toolbar';

describe('Toolbar', () => {
  it('renders without crashing', () => {
    render(<Toolbar />);
    expect(screen.getByRole('button', { name: /save/i })).toBeInTheDocument();
  });

  it('shows formatting buttons', () => {
    render(<Toolbar />);
    expect(screen.getByText('B')).toBeInTheDocument();
    expect(screen.getByText('I')).toBeInTheDocument();
    expect(screen.getByText('Code')).toBeInTheDocument();
  });

  it('shows save button as disabled when not dirty', () => {
    render(<Toolbar isDirty={false} />);
    const saveButton = screen.getByRole('button', { name: /save/i });
    expect(saveButton).toBeDisabled();
  });

  it('shows save button as enabled when dirty', () => {
    render(<Toolbar isDirty={true} />);
    const saveButton = screen.getByRole('button', { name: /save/i });
    expect(saveButton).not.toBeDisabled();
  });

  it('shows saving state', () => {
    render(<Toolbar isSaving={true} isDirty={true} />);
    expect(screen.getByText('Saving...')).toBeInTheDocument();
  });

  it('shows unsaved changes indicator when dirty', () => {
    render(<Toolbar isDirty={true} />);
    expect(screen.getByText('Unsaved changes')).toBeInTheDocument();
  });

  it('calls onSave when save button is clicked', () => {
    const mockOnSave = jest.fn();
    render(<Toolbar onSave={mockOnSave} isDirty={true} />);
    
    const saveButton = screen.getByRole('button', { name: /save/i });
    fireEvent.click(saveButton);
    
    expect(mockOnSave).toHaveBeenCalledTimes(1);
  });

  it('formatting buttons are disabled in phase 1', () => {
    render(<Toolbar />);
    const boldButton = screen.getByText('B').closest('button');
    const italicButton = screen.getByText('I').closest('button');
    const codeButton = screen.getByText('Code').closest('button');
    
    expect(boldButton).toBeDisabled();
    expect(italicButton).toBeDisabled();
    expect(codeButton).toBeDisabled();
  });
}); 