VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Form1"
   ClientHeight    =   885
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   5175
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   885
   ScaleWidth      =   5175
   StartUpPosition =   2  'CenterScreen
   Begin VB.CommandButton Command1 
      BackColor       =   &H00FFFFFF&
      Caption         =   "Crypt Code Section"
      Height          =   555
      Left            =   480
      MaskColor       =   &H00FFFFFF&
      Style           =   1  'Graphical
      TabIndex        =   0
      Top             =   180
      Width           =   4455
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Command1_Click()
    
    'crypt code section with simple xor
    
    Dim f As Long
    Dim p1 As String, p2 As String
    
    f = FreeFile
    
    p1 = App.path & "\pe_modified_hello.exe"
    p2 = App.path & "\crypted.exe"
    
    If Not FileExists(p1) Then
        MsgBox "ughh could not find modified pe sample:" & vbCrLf & p1, vbExclamation
        End
    End If
    
    If FileExists(p2) Then Kill p2
    
    FileCopy p1, p2
    
    Dim b As Byte
    Dim offset As Long
    Dim length As Long
    Dim StartAt As Long
    Dim i As Long
    
    StartAt = &H1048  'this is our programs original entry point
    length = &H2D86   '3DCE -1048  (original virtual size - entrypoint offset) = length to encrypt
    
    Open p2 For Binary As f
    
    For i = 1 To length
        offset = StartAt + i
        Get f, offset, b
        b = b Xor &HF
        Put f, offset, b
    Next
    
    Close f
    
    MsgBox "done"
    
End Sub




Function FileExists(path) As Boolean
  If Dir(path, vbHidden Or vbNormal Or vbReadOnly Or vbSystem) <> "" Then FileExists = True _
  Else FileExists = False
End Function

 
