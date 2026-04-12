import { Component } from '@angular/core';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatChipsModule } from '@angular/material/chips';
import { MatDividerModule } from '@angular/material/divider';
import { MatListModule } from '@angular/material/list';
import { MatSidenavModule } from '@angular/material/sidenav';
import { MatToolbarModule } from '@angular/material/toolbar';

import { JournalSpaceModel } from './tool-space.model';

@Component({
  selector: 'coretools-root',
  standalone: true,
  imports: [
    MatButtonModule,
    MatCardModule,
    MatChipsModule,
    MatDividerModule,
    MatListModule,
    MatSidenavModule,
    MatToolbarModule
  ],
  templateUrl: './app.component.html'
})
export class AppComponent {
  protected readonly title = 'coretools-angular';
  protected readonly activeSpace = new JournalSpaceModel();
  protected readonly topActions = [
    'File',
    'Edit',
    'Create',
    'Select',
    'Scene',
    '3D',
    'View',
    'Render',
    'Physics',
    'Animation',
    'Tools',
    'Build'
  ];
  protected readonly railItems = this.activeSpace.panels;
  protected readonly statusItems = ['Engine Ready', 'Dock Study', 'Angular Material'];
  protected readonly toolRibbonItems = this.activeSpace.actions;
  protected readonly bottomBarItems = this.activeSpace.status;
}
