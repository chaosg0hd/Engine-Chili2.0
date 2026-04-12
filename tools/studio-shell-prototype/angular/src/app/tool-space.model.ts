export type ToolSpaceKind = 'journal';

export interface ToolSpaceAction {
  readonly id: string;
  readonly label: string;
}

export interface ToolSpacePanel {
  readonly id: string;
  readonly label: string;
  readonly detail: string;
}

export interface ToolSpaceStatus {
  readonly label: string;
  readonly value: string;
}

export abstract class ToolSpaceModel {
  abstract readonly id: string;
  abstract readonly kind: ToolSpaceKind;
  abstract readonly name: string;
  abstract readonly label: string;
  abstract readonly eyebrow: string;
  abstract readonly summary: string;
  abstract readonly actions: readonly ToolSpaceAction[];
  abstract readonly panels: readonly ToolSpacePanel[];
  abstract readonly status: readonly ToolSpaceStatus[];
}

export class JournalSpaceModel extends ToolSpaceModel {
  override readonly id = 'journal-space';
  override readonly kind = 'journal';
  override readonly name = 'Journal Space';
  override readonly label = 'Journal';
  override readonly eyebrow = 'Abstract Model';
  override readonly summary = 'Working notes for design passes, system questions, and prototype decisions.';
  override readonly actions = [
    { id: 'capture', label: 'Capture' },
    { id: 'thread', label: 'Thread' },
    { id: 'link', label: 'Link' },
    { id: 'resolve', label: 'Resolve' }
  ] as const;
  override readonly panels = [
    { id: 'entries', label: 'Entries', detail: 'Timestamped notes and decisions' },
    { id: 'threads', label: 'Threads', detail: 'Grouped questions and follow-ups' },
    { id: 'references', label: 'References', detail: 'Links to scenes, assets, and modules' },
    { id: 'signals', label: 'Signals', detail: 'Open risks and next checks' }
  ] as const;
  override readonly status = [
    { label: 'Mode', value: 'Draft' },
    { label: 'Scope', value: 'Studio' },
    { label: 'Store', value: 'Memory' }
  ] as const;
}
